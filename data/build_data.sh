#!/usr/bin/env bash
set -euo pipefail

BASE_URL="https://mikaelsundell.s3.eu-west-1.amazonaws.com/flipman/data"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

urlencode_path() {
    python3 -c 'import sys, urllib.parse; print(urllib.parse.quote(sys.argv[1], safe="/"))' "$1"
}

echo "Downloading manifest:"
echo "  $BASE_URL/manifest.txt"

if ! curl -fL "$BASE_URL/manifest.txt" -o manifest.txt; then
    echo "ERROR: failed to download manifest:"
    echo "  $BASE_URL/manifest.txt"
    exit 1
fi

while IFS= read -r file; do
    [ -z "$file" ] && continue

    encoded_file="$(urlencode_path "$file")"
    url="$BASE_URL/$encoded_file"

    echo "Downloading:"
    echo "  file: $file"
    echo "  url:  $url"

    mkdir -p "$(dirname "$file")"

    if ! curl -fL "$url" -o "$file"; then
        echo
        echo "ERROR: failed to download:"
        echo "  file: $file"
        echo "  url:  $url"
        exit 1
    fi

    echo "OK: $file"
done < manifest.txt

echo "Data download complete."