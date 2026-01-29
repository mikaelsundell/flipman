#!/bin/bash

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
sdk_path="$(cd "$script_dir/../sources/flipmansdk" && pwd)"
apps_path="$(cd "$script_dir/../sources/flipman" && pwd)"
tests_path="$(cd "$script_dir/../sources/tests" && pwd)"

echo "Running clang-format in:"
echo "  $sdk_path"
echo "  $apps_path"
echo "  $tests_path"

find "$apps_path" "$sdk_path" "$tests_path" -maxdepth 5 \( -name '*.cpp' -o -name '*.h' \) | xargs clang-format -i

echo "Clang-format completed."
