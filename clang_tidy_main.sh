#!/usr/bin/env bash
set -euo pipefail

# Run clang-tidy on first-party C/C++ translation units under the repository
# root using the repository's .clang-tidy. Vendor code is excluded from both
# direct translation-unit checks and included-header diagnostics.
#
# The script intentionally does not read compile_commands.json from build*
# directories. Instead, it derives a generic host-side invocation from the
# active CC/CXX environment and a small set of repository-relative include
# roots.
#
# Usage:
#   ./clang_tidy_main.sh
#   ./clang_tidy_main.sh --fix

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$script_dir"
# no trailing slash: find's -path prune patterns below build on this and must
# exactly match the (single-slash) paths find constructs while traversing
main_dir="$repo_root"
vendor_dir="$repo_root/portable_concurrency"

if [[ ! -f "$repo_root/.clang-tidy" ]]; then
    echo "Error: .clang-tidy not found at repository root: $repo_root" >&2
    exit 1
fi

if [[ ! -d "$main_dir" ]]; then
    echo "Error: main directory not found: $main_dir" >&2
    exit 1
fi

if ! command -v clang-tidy >/dev/null 2>&1; then
    echo "Error: clang-tidy is not installed or not in PATH." >&2
    exit 1
fi

show_help() {
    cat <<'EOF'
Usage: ./clang_tidy_main.sh [--fix]

Options:
  --fix            Apply clang-tidy fix-its where possible.
  --help           Show this help text.

The script scans the repository root recursively for *.cpp and *.c files,
excluding build* directories and portable_concurrency/.

Headers are not passed to clang-tidy directly. clang-tidy is translation-unit
based, and running it on a header as the main file triggers false positives
such as '#pragma once in main file'. Header diagnostics are still reported when
those headers are included by checked source files.

The script uses CC for C files and CXX for C++ files. If CC/CXX are unset,
it falls back to cc/c++.
EOF
}

fix_mode="false"
cc_driver="${CC:-cc}"
cxx_driver="${CXX:-c++}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --fix)
            fix_mode="true"
            shift
            ;;
        --help|-h)
            show_help
            exit 0
            ;;
        *)
            echo "Error: unknown argument: $1" >&2
            show_help >&2
            exit 1
            ;;
    esac
done

if ! command -v "$cc_driver" >/dev/null 2>&1; then
    echo "Error: CC compiler not found: $cc_driver" >&2
    exit 1
fi

if ! command -v "$cxx_driver" >/dev/null 2>&1; then
    echo "Error: CXX compiler not found: $cxx_driver" >&2
    exit 1
fi

collect_system_include_args() {
    local compiler="$1"
    local language="$2"
    local line=""
    local in_block="false"
    local include_args=()

    while IFS= read -r line; do
        if [[ "$line" == "#include <...> search starts here:" ]]; then
            in_block="true"
            continue
        fi

        if [[ "$line" == "End of search list." ]]; then
            break
        fi

        if [[ "$in_block" == "true" ]]; then
            line="${line#"${line%%[![:space:]]*}"}"
            if [[ -n "$line" ]]; then
                include_args+=("-isystem" "$line")
            fi
        fi
    done < <("$compiler" -E -x "$language" - -v < /dev/null 2>&1)

    printf '%s\0' "${include_args[@]}"
}

mapfile -d '' c_system_include_args < <(collect_system_include_args "$cc_driver" c)
mapfile -d '' cxx_system_include_args < <(collect_system_include_args "$cxx_driver" c++)

common_include_args=("-I$main_dir")

if [[ -d "$main_dir/tests" ]]; then
    common_include_args+=("-I$main_dir/tests")
fi

if [[ -d "$repo_root/_deps/googletest-src/googletest/include" ]]; then
    common_include_args+=("-I$repo_root/_deps/googletest-src/googletest/include")
fi

if [[ -d "$repo_root/_deps/googletest-src/googlemock/include" ]]; then
    common_include_args+=("-I$repo_root/_deps/googletest-src/googlemock/include")
fi

c_target="$("$cc_driver" -dumpmachine 2>/dev/null || true)"
cxx_target="$("$cxx_driver" -dumpmachine 2>/dev/null || true)"
host_is_linux="false"

if [[ "$c_target" == *linux* || "$cxx_target" == *linux* ]]; then
    host_is_linux="true"
fi

mapfile -d '' files < <(
    find "$main_dir" \
        \( -type d -name 'build*' -prune \) -o \
        \( -type d -path "$vendor_dir" -prune \) -o \
        \( -type d -path "$main_dir/tools/freertos" -prune \) -o \
        \( -type d -path "$main_dir/tests" -prune \) -o \
        \( -type f \( -name '*.cpp' -o -name '*.c' \) -print0 \)
)

if [[ ${#files[@]} -eq 0 ]]; then
    echo "No matching files found under $main_dir"
    exit 0
fi

tidy_args=()
if [[ "$fix_mode" == "true" ]]; then
    tidy_args+=("--fix")
fi

failure_count=0
for file_path in "${files[@]}"; do
    if [[ "$host_is_linux" != "true" && "$file_path" == "$main_dir/tools/linux/"* ]]; then
        echo "Skipping Linux-specific file on non-Linux host: $file_path"
        continue
    fi

    echo "Checking $file_path"

    if [[ "$file_path" == *.c || "$file_path" == *.h ]]; then
        file_args=("-xc" "-std=gnu11")
        file_args+=("${common_include_args[@]}")
        file_args+=("${c_system_include_args[@]}")
    else
        file_args=("-xc++" "-std=gnu++20")
        file_args+=("${common_include_args[@]}")
        file_args+=("${cxx_system_include_args[@]}")
    fi

    if ! (cd "$repo_root" && clang-tidy "${tidy_args[@]}" \
        --header-filter='^(?!.*portable_concurrency/).*\.hpp$' \
        "$file_path" -- "${file_args[@]}"); then
        failure_count=$((failure_count + 1))
    fi
done

if [[ $failure_count -gt 0 ]]; then
    echo "clang-tidy reported issues in $failure_count file(s)." >&2
    exit 1
fi

echo "clang-tidy completed successfully for ${#files[@]} file(s)."
