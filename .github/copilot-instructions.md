# GitHub Copilot Instructions

## Quick Context

This is the desktop PublishSubscribe project (CMake + C++17 baseline, with C++20/C++23 compatibility).

First-party code:

- `tools/`
- `main.cpp`
- `CMakeLists.txt`, `README.md`

Vendor code:

- `portable_concurrency/` (edit only when explicitly requested)

## What To Edit By Default

- Prefer `tools/` and `main.cpp`.
- Keep edits minimal and targeted.
- Avoid wide refactors unless requested.

## Compatibility Rules

- Keep code compatible with C++17.
- Preserve C++20/C++23 support when present.
- For newer-language features, add guards and fallbacks when practical.

Example guard pattern:

```cpp
#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
// C++20 path
#else
// C++17 fallback
#endif
```

## Build/Run Reference

- Configure: `cmake -S . -B build`
- Build: `cmake --build build -j`
- Run: `./build/publish_subscribe`

## Coding Conventions

- Follow `.clang-format` and `.clang-tidy`.
- Prefer snake_case naming.
- Prefer descriptive identifiers over short names.
- Prefer `const`, `constexpr`, and named constants.
- Use project include style consistently (`"..."` for project headers, `<...>` for STL/system).

## API and Design Preferences

- Prefer RAII and clear ownership semantics.
- Prefer STL containers/algorithms over custom low-level code when suitable.
- Use `tools/expected` for new result-style APIs where appropriate.
- Keep public APIs simple, explicit, and consistent with existing `tools/*` style.

## Concurrency Preferences

- Follow existing thread-safe container and task patterns already in `tools/`.
- Reuse project abstractions before introducing new primitives.
- Keep async examples consistent with local `portable_concurrency` wrapper APIs.

## Documentation and Validation

- If behavior or API changes, update `README.md` and relevant examples.
- Validate modified files for diagnostics.
- Build for non-trivial changes when feasible.
