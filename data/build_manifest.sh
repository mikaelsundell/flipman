#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

find . -type f \
    ! -name ".DS_Store" \
    ! -name "._*" \
    ! -name "*.sh" \
    ! -name "manifest.txt" \
    | sed 's#^\./##' \
    | sort > manifest.txt

echo "Wrote manifest.txt"