#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# Adapted from Mikael Sundell's Stageviz deployment workflow.
set -euo pipefail
usage() { echo "usage: $0 --bundle App.app --dmg output.dmg"; }
bundle=""; dmg=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help) usage; exit 0 ;;
        -b|--bundle|-d|--dmg)
            [[ $# -ge 2 ]] || { usage >&2; exit 2; }
            if [[ "$1" == -b || "$1" == --bundle ]]; then bundle="$2"; else dmg="$2"; fi
            shift 2 ;;
        *) usage >&2; exit 2 ;;
    esac
done
[[ -d "$bundle/Contents" && -n "$dmg" ]] || { usage >&2; exit 2; }
[[ ! -e "$dmg" ]] || { echo "Output already exists: $dmg" >&2; exit 1; }
mkdir -p "$(dirname "$dmg")"
staging=$(mktemp -d "${TMPDIR:-/tmp}/flipman-dmg.XXXXXX")
trap 'rm -rf "$staging"' EXIT
ditto "$bundle" "$staging/$(basename "$bundle")"
ln -s /Applications "$staging/Applications"
cat > "$staging/GitHub project.webloc" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict><key>URL</key><string>https://github.com/mikaelsundell/flipman</string></dict></plist>
PLIST
# No mounted writable image or project-specific Finder metadata is required.
hdiutil create -srcfolder "$staging" -volname Flipman -fs HFS+ -format UDZO "$dmg"
echo "Created $dmg"
