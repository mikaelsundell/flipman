#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# Configure, build and package Flipman.
set -euo pipefail
root=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
usage() {
    cat <<'HELP'
usage: build_app.sh debug|release|all [options]
  --configure-only       Generate the Xcode project without building
  --fresh                Discard cached CMake configuration before generating
  --sdk-only             Build only the SDK, bindings and tests
  --python               Build SDK Python bindings
  --braw                 Build the Blackmagic RAW reader (SDK root required)
  --target=VERSION       Minimum macOS version (not the SDK version)
  --deploy, --github     Package a bundle into deploy.CONFIG (ZIP)
  --bundle=PATH          App bundle to package (explicit development bundle)
  --dmg                  Also create a DMG (implies --deploy)
  --sign                 Sign with DEVELOPERID_IDENTITY (otherwise ad-hoc)
Environment:
  THIRDPARTY_DIR         Matching dependency prefix for one configuration
  THIRDPARTY_DEBUG_DIR / THIRDPARTY_RELEASE_DIR   Prefixes for 'all'
  DeckLinkSDK_ROOT       Blackmagic DeckLink SDK root (required for demos)
  BLACKMAGIC_RAW_ROOT    Optional RAW SDK root with Mac/Libraries runtime
  BLACKMAGIC_RAW_LICENSE Accompanying RAW runtime license/notice to include
HELP
}
mode=""; fresh=0; configure_only=0; application=ON
demos=ON; python=OFF; braw=OFF; deploy=0; dmg=0; sign=0; bundle=""; target="${MACOSX_DEPLOYMENT_TARGET:-}"
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help) usage; exit 0 ;;
        debug|release|all) [[ -z "$mode" ]] || { usage >&2; exit 2; }; mode="$1" ;;
        --configure-only) configure_only=1 ;;
        --fresh) fresh=1 ;;
        --sdk-only) demos=OFF; application=OFF ;;
        --python) python=ON ;;
        --braw) braw=ON ;;
        --target=*) target="${1#*=}" ;;
        --deploy|--github) deploy=1 ;;
        --dmg) deploy=1; dmg=1 ;;
        --sign) sign=1 ;;
        --bundle=*) bundle="${1#*=}" ;;
        *) echo "Unknown argument: $1" >&2; usage >&2; exit 2 ;;
    esac
    shift
done
[[ -n "$mode" ]] || { usage >&2; exit 2; }
# Default to the running OS major, independently of the SDK.
if [[ -z "$target" ]]; then
    target=$(sw_vers -productVersion)
    target="${target%%.*}"
fi
if ! command -v cmake >/dev/null 2>&1; then export PATH="/Applications/CMake.app/Contents/bin:$PATH"; fi
command -v cmake >/dev/null || { echo "CMake 3.27+ is required" >&2; exit 1; }
if [[ "$deploy" == 1 ]]; then
    [[ "$configure_only" == 0 && "$mode" != all && -d "$bundle/Contents" ]] || {
        echo "Deployment requires a single configuration and --bundle=/path/to/App.app; Standalone Python runtime packaging is not yet integrated." >&2; exit 2;
    }
fi
if [[ "$sign" == 1 && -z "${DEVELOPERID_IDENTITY:-}" ]]; then
    echo "--sign requires DEVELOPERID_IDENTITY" >&2; exit 2
fi
if [[ "$mode" == all && ( -z "${THIRDPARTY_DEBUG_DIR:-}" || -z "${THIRDPARTY_RELEASE_DIR:-}" ) ]]; then
    echo "all requires THIRDPARTY_DEBUG_DIR and THIRDPARTY_RELEASE_DIR" >&2; exit 2
fi
build_one() (
    local config="$1" prefix configuration
    if [[ "$config" == debug ]]; then
        configuration=Debug
        prefix="${THIRDPARTY_DEBUG_DIR:-${THIRDPARTY_DIR:-}}"
        export DYLD_IMAGE_SUFFIX=_debug
    else
        configuration=Release
        prefix="${THIRDPARTY_RELEASE_DIR:-${THIRDPARTY_DIR:-}}"
        unset DYLD_IMAGE_SUFFIX
    fi
    [[ -d "$prefix" && -n "$prefix" ]] || { echo "Set the matching THIRDPARTY_DEBUG_DIR / THIRDPARTY_RELEASE_DIR or THIRDPARTY_DIR" >&2; exit 2; }
    local args=(-S "$root" -B "$root/build.$config" -G Xcode
        -DCMAKE_OSX_SYSROOT=macosx -DCMAKE_CONFIGURATION_TYPES="$configuration" -DCMAKE_PREFIX_PATH="$prefix"
        -DFLIPMAN_BUILD_APP="$application" -DFLIPMAN_BUILD_DEMOS="$demos" -DFLIPMAN_BUILD_PYTHON="$python" -DFLIPMAN_BUILD_BRAW="$braw")
    if [[ -n "${BLACKMAGIC_RAW_ROOT:-}" ]]; then args+=(-DBlackmagicRaw_ROOT="$BLACKMAGIC_RAW_ROOT"); fi
    export MACOSX_DEPLOYMENT_TARGET="$target"
    args+=(-DCMAKE_OSX_DEPLOYMENT_TARGET="$target")
    if [[ -n "${DeckLinkSDK_ROOT:-}" ]]; then args+=(-DDeckLinkSDK_ROOT="$DeckLinkSDK_ROOT"); fi
    if [[ "$fresh" == 1 ]]; then args+=(--fresh); fi
    cmake "${args[@]}"
    if [[ "$configure_only" == 1 ]]; then exit 0; fi
    cmake --build "$root/build.$config" --config "$configuration" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-6}"
    if [[ "$deploy" == 1 ]]; then
        local package_args=(--bundle "$bundle" --prefix "$prefix" --output "$root/deploy.$config" --config "$configuration")
        if [[ "$dmg" == 1 ]]; then package_args+=(--dmg); fi
        if [[ "$sign" == 1 ]]; then package_args+=(--sign); fi
        "$root/scripts/package_app.sh" "${package_args[@]}"
    fi
)
if [[ "$mode" == all ]]; then build_one debug; build_one release; else build_one "$mode"; fi
