#!/usr/bin/env bash
set -euo pipefail

# Format C/C++ files under the repository root using the repository's .clang-format.
# Usage:
#   ./format_main.sh            # format in place
#   ./format_main.sh --check    # report files that would change

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$script_dir"
main_dir="$repo_root/"

if [[ ! -f "$repo_root/.clang-format" ]]; then
    echo "Error: .clang-format not found at repository root: $repo_root" >&2
    exit 1
fi

if [[ ! -d "$main_dir" ]]; then
    echo "Error: main directory not found: $main_dir" >&2
    exit 1
fi

if ! command -v clang-format >/dev/null 2>&1; then
    echo "Error: clang-format is not installed or not in PATH." >&2
    exit 1
fi

mode="format"
if [[ "${1:-}" == "--check" ]]; then
    mode="check"
elif [[ -n "${1:-}" ]]; then
    echo "Usage: $0 [--check]" >&2
    exit 1
fi

mapfile -d '' files < <(
    find "$main_dir" \
        \( -type d -name 'build*' -prune \) -o \
        \( -type f \( -name '*.cpp' -o -name '*.c' -o -name '*.h' -o -name '*.hpp' -o -name '*.inl' \) -print0 \)
)

if [[ ${#files[@]} -eq 0 ]]; then
    echo "No matching files found under $main_dir"
    exit 0
fi

if [[ "$mode" == "check" ]]; then
    changed_count=0
    for file_path in "${files[@]}"; do
        if ! clang-format --style=file --dry-run --Werror "$file_path" >/dev/null 2>&1; then
            echo "$file_path"
            changed_count=$((changed_count + 1))
        fi
    done

    if [[ $changed_count -gt 0 ]]; then
        echo "Found $changed_count file(s) that need formatting."
        exit 1
    fi

    echo "All checked files are correctly formatted."
    exit 0
fi

for file_path in "${files[@]}"; do
    clang-format --style=file -i "$file_path"
done

echo "Formatted ${#files[@]} file(s) under $main_dir"
