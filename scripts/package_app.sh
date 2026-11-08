#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# Adapted from Stageviz's build_app.sh/deploymac/deploydmg workflow.
set -euo pipefail
root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
usage() { echo "usage: $0 --bundle App.app --prefix DEPENDENCIES --output DIR --config Debug|Release [--dmg] [--sign]"; }
bundle=""; prefix=""; output=""; config=""; dmg=0; sign=0
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help) usage; exit 0 ;;
        --bundle|--prefix|--output|--config)
            [[ $# -ge 2 ]] || { usage >&2; exit 2; }
            case "$1" in
                --bundle) bundle="$2" ;; --prefix) prefix="$2" ;;
                --output) output="$2" ;; --config) config="$2" ;;
            esac
            shift 2 ;;
        --dmg) dmg=1; shift ;;
        --sign) sign=1; shift ;;
        *) usage >&2; exit 2 ;;
    esac
done
[[ -d "$bundle/Contents" && -d "$prefix" && -n "$output" ]] || { usage >&2; exit 2; }
case "$config" in Debug) export DYLD_IMAGE_SUFFIX=_debug ;; Release) unset DYLD_IMAGE_SUFFIX ;; *) usage >&2; exit 2 ;; esac
[[ -x "$prefix/bin/macdeployqt" ]] || { echo "Missing $prefix/bin/macdeployqt" >&2; exit 1; }
if [[ "$sign" == 1 && -z "${DEVELOPERID_IDENTITY:-}" ]]; then echo "Set DEVELOPERID_IDENTITY" >&2; exit 2; fi
raw_framework=""
if [[ -n "${BLACKMAGIC_RAW_ROOT:-}" ]]; then
    for path in "$BLACKMAGIC_RAW_ROOT/Mac/Libraries/BlackmagicRawAPI.framework" "$BLACKMAGIC_RAW_ROOT/BlackmagicRawAPI.framework"; do
        if [[ -d "$path" ]]; then raw_framework="$path"; break; fi
    done
    [[ -n "$raw_framework" ]] || { echo "Blackmagic RAW framework not found under BLACKMAGIC_RAW_ROOT" >&2; exit 1; }
    [[ -f "${BLACKMAGIC_RAW_LICENSE:-}" ]] || { echo "Set BLACKMAGIC_RAW_LICENSE to the SDK's runtime redistribution license/notice" >&2; exit 1; }
fi
version=$(sed -n 's/^[[:space:]]*set[[:space:]]*(project_long_version "\([^"]*\)").*/\1/p' "$root/CMakeLists.txt" | head -1)
[[ -n "$version" ]] || { echo "Project version not found" >&2; exit 1; }
mkdir -p "$output"
output=$(cd "$output" && pwd)
name=$(basename "$bundle")
[[ "$name" == *.app ]] || { echo "Expected an .app bundle" >&2; exit 2; }
stem="Flipman_${version}_$(date +%y%m%d)_$(uname -m)_$config"
[[ ! -e "$output/$name" && ! -e "$output/$stem.zip" && ! -e "$output/$stem.dmg" ]] || {
    echo "Output already contains this bundle or archive: $output" >&2; exit 1;
}
ditto "$bundle" "$output/$name"
app="$output/$name"
mkdir -p "$app/Contents/Resources/Licenses" "$app/Contents/Frameworks"
cp "$root/LICENSE" "$app/Contents/Resources/Licenses/Flipman.txt"
cp "$root/3rdparty/glsl-parser/LICENSE" "$app/Contents/Resources/Licenses/glsl-parser.txt"
cp "$root/docs/blackmagic-runtime.md" "$app/Contents/Resources/Blackmagic-runtime.md"
if [[ -n "$raw_framework" ]]; then
    ditto "$raw_framework" "$app/Contents/Frameworks/BlackmagicRawAPI.framework"
    cp "$BLACKMAGIC_RAW_LICENSE" "$app/Contents/Resources/Licenses/BlackmagicRAW.txt"
fi
if [[ -f "${DeckLinkSDK_ROOT:-}/Mac/include/DeckLinkAPIDispatch.cpp" ]]; then
    sed -n '/-LICENSE-START-/,/-LICENSE-END-/p' "$DeckLinkSDK_ROOT/Mac/include/DeckLinkAPIDispatch.cpp" \
        > "$app/Contents/Resources/Licenses/DeckLink.txt"
fi
qt_args=("$app")
if [[ "$config" == Debug ]]; then qt_args+=(-debug -no-strip); fi
"$prefix/bin/macdeployqt" "${qt_args[@]}"
args=(--bundle "$app" --prefix "$prefix")
if [[ "$sign" == 1 ]]; then args+=(--sign --identity "$DEVELOPERID_IDENTITY"); fi
"$root/scripts/deploymac.sh" "${args[@]}"
"$root/scripts/deploycheck.sh" "$app"
codesign --verify --deep --strict "$app"
ditto -c -k --sequesterRsrc --keepParent "$app" "$output/$stem.zip"
if [[ "$dmg" == 1 ]]; then "$root/scripts/deploydmg.sh" --bundle "$app" --dmg "$output/$stem.dmg"; fi
echo "Packaged $app"
