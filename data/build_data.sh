#!/usr/bin/env bash
set -euo pipefail

BASE_URL="https://mikaelsundell.s3.eu-west-1.amazonaws.com/flipman/data"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

curl -fL "$BASE_URL/manifest.txt" -o manifest.txt

while IFS= read -r file; do
    [ -z "$file" ] && continue

    mkdir -p "$(dirname "$file")"
    curl -fL "$BASE_URL/$file" -o "$file"
done < manifest.txt

echo "Data download complete"