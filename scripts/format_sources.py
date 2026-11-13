#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 Mikael Sundell

"""Format first-party C++ and Objective-C++ using Flipman's layout conventions."""

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parent.parent
# Protect comments and literals, including embedded GLSL, during spacing edits.
LITERALS = re.compile(
    r'R"(?P<delimiter>[^ ()\\\t\r\n]{0,16})\(.*?\)(?P=delimiter)"'
    r'|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    r'|//[^\n]*|/\*.*?\*/',
    re.DOTALL,
)


def spacing(source):
    mask = LITERALS.sub(lambda match: re.sub(r"[^\n]", " ", match.group()), source)
    lines = source.splitlines(keepends=True)
    code = mask.splitlines(keepends=True)
    before = set()
    for i, line in enumerate(lines):
        if i == 0 or not line.strip() or not lines[i - 1].strip():
            continue
        previous = lines[i - 1].strip()
        current = line.strip()
        # Keep the file banner, pragma, and include block visually distinct.
        directive = code[i].lstrip().startswith("#")
        if (
            (previous.startswith("//") and directive)
            or code[i].strip() == "#pragma once"
            or code[i - 1].strip() == "#pragma once"
        ):
            before.add(i)
        if current.startswith("//") and code[i - 1].rstrip().endswith((";", "}")):
            before.add(i)
        # clang-format preserves these gaps but does not insert them around
        # namespace aliases or immediately nested namespaces.
        if re.match(r"\s*namespace\b.*\{\s*$", code[i - 1]):
            before.add(i)
        if code[i].lstrip().startswith("}") and re.match(r"\s*}\s*//\s*namespace\b", line):
            before.add(i)
    # Separate guard returns from the next independent operation, including
    # wrapped return expressions. Do not split switch cases or closing braces.
    for match in re.finditer(r"(?m)^([ \t]*)return\b[^;{}]*;\n(?=[ \t]*[A-Za-z_])", mask):
        i = mask.count("\n", 0, match.end())
        following = code[i].lstrip()
        indent = len(code[i]) - len(code[i].lstrip())
        if indent <= len(match.group(1)) and not re.match(r"(?:case|default|else)\b", following):
            before.add(i)
    return "".join(("\n" if i in before else "") + line for i, line in enumerate(lines))


def compact_data(source):
    """Keep Data fields compact without modifying literals or sorting lifetimes."""
    literals = list(LITERALS.finditer(source))
    mask = LITERALS.sub(lambda match: re.sub(r"[^\n]", " ", match.group()), source)
    removals = []
    for match in re.finditer(r"\bstruct\s+Data\s*\{", mask):
        depth = 1
        end = match.end()
        while depth and end < len(mask):
            depth += (mask[end] == "{") - (mask[end] == "}")
            end += 1
        for blank in re.finditer(r"(?m)^[ \t]*\n", source[match.end():end]):
            start = match.end() + blank.start()
            if source[start - 1:start] == "\n" and not any(
                literal.start() <= start < literal.end() for literal in literals
            ):
                removals.append((start, match.end() + blank.end()))
        # clang-format's SeparateDefinitionBlocks adds this gap; the repository
        # explicitly keeps Data and its instance together instead.
        instance = re.match(r";\n([ \t]*\n)+(?=[ \t]*Data d;)", mask[end:])
        if instance:
            removals.append((end + 2, end + instance.end()))
    for start, end in sorted(set(removals), reverse=True):
        source = source[:start] + source[end:]
    return source


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="report files needing formatting without writing")
    parser.add_argument("paths", nargs="*", help="files or directories, relative to the repository root")
    args = parser.parse_args()
    executable = os.environ.get("CLANG_FORMAT")
    if not executable and sys.platform == "darwin":
        result = subprocess.run(["xcrun", "--find", "clang-format"], capture_output=True, text=True)
        if result.returncode == 0:
            executable = result.stdout.strip()
    executable = executable or shutil.which("clang-format")
    if not executable:
        parser.error("clang-format 21 or newer is required; set CLANG_FORMAT to its executable")
    version = subprocess.check_output([executable, "--version"], text=True)
    match = re.search(r"version (\d+)", version)
    if not match or int(match.group(1)) < 21:
        parser.error("clang-format 21 or newer is required; set CLANG_FORMAT to its executable")
    paths = set()
    for name in args.paths or ["sources", "CTest"]:
        path = (ROOT / name).resolve()
        if not path.exists():
            parser.error("path does not exist: " + name)
        for candidate in path.rglob("*") if path.is_dir() else [path]:
            if candidate.is_file() and candidate.suffix in (".h", ".cpp", ".mm", ".m"):
                relative = candidate.relative_to(ROOT)
                if relative.parts[0] in ("sources", "CTest"):
                    paths.add(candidate)
    changed = 0
    for path in sorted(paths):
        source = path.read_text(encoding="utf-8")
        # A C++ filename hint lets the C++ style cover Objective-C++ too.
        hint = path.with_suffix(".cpp") if path.suffix in (".mm", ".m") else path
        formatted = subprocess.check_output(
            [executable, "--assume-filename=" + str(hint)], input=spacing(source), text=True, encoding="utf-8"
        )
        formatted = compact_data(formatted)
        if formatted != source:
            changed += 1
            if args.check:
                print(path.relative_to(ROOT))
            else:
                path.write_text(formatted, encoding="utf-8")
    action = "need" if args.check else "updated by"
    print("{} files checked; {} {} formatting.".format(len(paths), changed, action))
    return 1 if args.check and changed else 0


if __name__ == "__main__":
    sys.exit(main())
