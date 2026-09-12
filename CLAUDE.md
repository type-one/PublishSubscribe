# GitHub Copilot Instructions

## Repository Intent

This repository contains a lightweight and portable Publish/Subscribe framework for desktop builds.

Primary first-party code lives in:

- `tools/`
- `main.cpp`
- root build files and docs (`CMakeLists.txt`, `README.md`)

Vendored or externally sourced code:

- `portable_concurrency/`

Treat `portable_concurrency/` as vendor code by default. Avoid broad edits there unless explicitly requested.

## Edit Scope Rules

- Prefer edits in first-party code (`tools/`, `main.cpp`, root build glue/docs).
- Do not modify vendored third-party code unless explicitly requested.
- Keep changes localized and minimal.

## C++ Version Policy

- Baseline compatibility is C++17.
- C++20 and C++23 compatibility should be preserved.
- Use feature checks when using newer features:
  - `__cplusplus`
  - `_MSVC_LANG`
  - feature-test macros where relevant.
- Provide C++17-compatible fallbacks for C++20/C++23-only APIs when practical.

## Build and Validation

Primary desktop build flow:

- Configure: `cmake -S . -B build`
- Build: `cmake --build build -j`
- Run: `./build/publish_subscribe` (Linux)

Windows flow (existing generated tree may exist):

- Configure with CMake/Visual Studio generator.
- Build the `publish_subscribe` target.

When changing code:

- Keep formatting/style aligned with `.clang-format` and `.clang-tidy`.
- Run diagnostics on modified files.
- Prefer validating by building after non-trivial changes.

## Style and Naming

- Follow repository `.clang-format` and `.clang-tidy`.
- Prefer snake_case for functions/methods/variables.
- Use descriptive names (avoid very short names except tiny loop indices).
- Prefix class members with `m_` when adding new classes with state.
- Prefer `const` and `constexpr` when applicable.
- Avoid magic numbers; prefer named constants.

Include style:

- `#include "..."` for project headers.
- `#include <...>` for STL/system headers.

## Standard Library and Safety Guidance

- Prefer standard library containers and algorithms.
- Prefer `std::string` / `std::string_view` over raw C strings when ownership and
  lifetime are clear.
- Prefer `std::span<T>` (C++20) over raw pointer-and-length pairs or array
  parameters for non-owning views over contiguous data. Guard it with a C++17
  fallback such as `const T*` plus `size_t`, following the C++ Version Policy.
- Prefer `std::format` (C++20) over `printf`/`snprintf`-style formatting in new
  code for type-safe, composable text formatting. In C++17 code, use `snprintf`
  or an existing project helper behind the same feature checks.
- Prefer `<algorithm>` functions and range-based `for` loops when they make the
  intent clearer than hand-written or index-based loops.
- Prefer RAII and avoid manual ownership with raw `new`/`delete` in new code.
- Use `std::unique_ptr`/`std::shared_ptr` when ownership is required.
- Keep APIs explicit and readable; avoid ambiguous forwarding overloads.

## API and Ownership Guidance

- Pass scalar values by value and heavier read-only objects by `const T&`.
- Pass `std::string_view` and `std::span<T>` by value; both are small, cheap
  non-owning views and must not be used to extend or transfer ownership.
- When a constructor stores a `std::shared_ptr<T>` to express shared ownership,
  take it by value and move it into the member. Use `const std::shared_ptr<T>&`
  when the callee only observes the pointer during the call.
- Move movable by-value constructor inputs into members.
- Avoid passing `std::string_view` or `std::span<T>` by `const&`, owning raw
  pointers, unnecessary copies at API boundaries, and implementation bodies for
  non-templated classes inline in headers.

## Concurrency Guidance

- In first-party code, prefer existing abstractions in `tools/` and `portable_concurrency/`.
- Keep synchronization and task orchestration consistent with existing patterns used by:
  - `tools/sync_*` containers
  - `tools/worker_task.hpp`
  - `tools/periodic_task.hpp`
  - `portable_concurrency` futures/executors helpers

## Documentation Conventions

- Keep README and examples aligned with implemented behavior.
- If API behavior changes, update docs and usage examples in the same change where practical.
- Keep comments focused on non-obvious intent (`why`), not line-by-line narration (`what`).

## Error Handling Guidance

- Prefer explicit error handling and clear result reporting.
- `tools/expected` is preferred for new result-based APIs where applicable.
- Do not introduce exceptions in new APIs unless there is a strong, explicit reason.

## Preferred Contribution Pattern

1. Read nearby code in `tools/` and related demos in `main.cpp`.
2. Implement a minimal, localized change.
3. Preserve C++17/20/23 compatibility as documented in README.
4. Validate modified files and build when appropriate.
5. Update docs/examples if user-facing behavior changed.
