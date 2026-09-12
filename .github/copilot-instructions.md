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
- Use project include style consistently (`"..."` for project headers, `<...>` for STL/system).

## API Design Preferences

- Use `tools/expected` for new result-style APIs where appropriate.
- Keep public APIs simple, explicit, and consistent with existing `tools/*` style.

## Formatting and Static Analysis Source of Truth

- Formatting rules are governed by the root `.clang-format` file.
- Static-analysis rules are governed by the root `.clang-tidy` file when
	present.
- Consult those files directly rather than duplicating their detailed rule
	lists in repository guidance.
- When formatting code, use the repository's `format_main.sh` script when it is
	applicable to the changed files.

## Style and Naming Details

- Use variable and parameter names with at least three characters, except for
	conventional loop indices in very small local scopes.
- Prefix class data members with `m_`.
- Enforce constness whenever possible: prefer `const` methods, `const auto`
	for intermediate computations, and `constexpr` for compile-time values.
- Do not use magic numbers; replace them with named constants, preferably
	`constexpr` where appropriate.
- If a class has private members, include an explicit `private:` section.
- Avoid `protected:` sections unless the design explicitly requires them.
- Mark every overriding virtual method with `override`; use `final` only when
	extension must be prohibited by design.

## Standard Library Usage

- Prefer modern standard-library facilities over C-style techniques when they
	fit the platform constraints.
- Prefer STL containers and the facilities in `tools/` over custom data
	structures when they provide the required behavior.
- Prefer `auto` when it removes noisy type spellings without obscuring meaning;
	use explicit types when deduction could hide proxy, ownership, or narrowing
	behavior.
- Prefer `constexpr` variables and functions over macros for compile-time
	constants and computations.
- Prefer brace initialization when it improves clarity and avoids narrowing;
	use parentheses deliberately when initializer-list overloads could surprise.
- In C++20 code, use `<ranges>` pipelines when they make data processing more
	expressive, and provide an equivalent C++17 `<algorithm>` path behind the
	repository's feature checks.
- Do not re-implement functionality the STL already provides correctly and
	portably.
- Prefer `emplace_*` only when it clearly avoids temporary materialization and
	preserves readability; otherwise use `push_*` or `insert`.

## Template API Design

- Follow the established style in `tools/*.hpp`: keep exact-type overloads for
	explicit common call paths and add perfect-forwarding overloads only when
	they provide real value.
- Forward forwarding references with `std::forward`; use `std::move` for named
	rvalue-reference or by-value inputs, not for forwarding references.
- Constrain forwarding overloads with `requires` in C++20 and an equivalent
	SFINAE fallback in C++17 to avoid ambiguous or overly generic matches.
- Avoid overload sets that combine unconstrained forwarding-reference
	templates with nearby non-template overloads.
- Prefer templated callables or `auto` parameters over `std::function` when
	callability is needed without type erasure; keep `std::function` when the
	stored callback contract requires type erasure.

## Object Lifetime and API Signatures

- Use RAII for memory, locks, file handles, timers, and other acquire/release
	lifecycles.
- Apply the Rule of Five when ownership semantics require it, or inherit from
	`tools::non_copyable` when copy and move must be disallowed.
- Use smart pointers to encode ownership: `std::unique_ptr` for unique
	ownership and `std::shared_ptr` for shared ownership. Raw pointers and
	references are permitted only for justified non-owning access.
- Pass simple values by value, movable types by value when a local owning copy
	is intended, and heavier read-only objects by `const T&`.
- Pass `std::string_view` and `std::span<T>` by value.
- When a constructor stores a `std::shared_ptr<T>` as a shared-ownership member,
	take it by value and move it into the member. A `const std::shared_ptr<T>&` is
	appropriate when the callee only observes the pointee during the call.
- Keep non-templated implementation bodies out of headers where practical;
	template implementations necessarily remain in headers.

## OOP and Clean Code

- Respect information hiding, encapsulation, and the Liskov substitution
	principle in polymorphic APIs.
- Do not introduce singletons or global variables unless a narrow, concrete
	requirement justifies them.
- Prefer fixing root causes over layered workarounds.
- Use meaningful and searchable names instead of unnecessary abbreviations.
- Comments should explain why, not restate what the code already says.
- Keep each class focused on one responsibility and methods short and focused.
- Prefer explanatory intermediate variables, early returns, and positive
	conditionals when they improve readability.
- Avoid boolean parameter traps; use named enums or separate overloads when a
	boolean controls behavior.
- Avoid deeply nested lambdas. Extract non-trivial logic into named lambdas,
	helper functions, or file-local functions in an anonymous namespace.
- Keep logical expressions and cyclomatic complexity reasonable.

## Concurrency and Platform Abstractions

- Reuse the synchronization and task abstractions already present in `tools/`
	and `portable_concurrency/` before introducing new primitives.
- Prefer `tools::worker_task`, `tools::periodic_task`, `tools::sync_object`, and
	the `tools::sync_*` containers for framework-level asynchronous work and
	synchronization when they match the requirement.
- Keep direct `std::thread`, mutex, and condition-variable usage consistent with
	existing code and limited to cases where the local abstractions do not fit.
- Keep asynchronous callbacks and queue operations small; do not perform
	unnecessary blocking, allocation-heavy work, or complex business logic in
	latency-sensitive paths.
- `tools::async_observer` reports entries dropped by bounded queues through
	`has_queue_overflow()`, `queue_overflow_count()`, and
	`consume_queue_overflow_count()`. Components using bounded observers should
	poll the consumed count and publish an explicit notification when dropped
	events matter to the application.

## Design Patterns

### Messages and Events

- Group related messages, commands, or events into a `std::variant` when the
	domain has a closed set of alternatives.
- Dispatch variants with `std::visit` and focused overloads or lambdas instead
	of chains on type tags or discriminant strings.
- Prefer `enum class` or structs with named fields so the alternative type carries
	semantic meaning.

### Finite State Machines

- When an explicit finite state machine is needed, model states as a
	`std::variant` of distinct state structs.
- Dispatch events with `std::visit` and a local overload pattern. Give each
	meaningful state/event combination a focused handler and make unhandled
	combinations explicit.

### Publish/Subscribe

- Use `tools::sync_observer` or `tools::async_observer` for observer behavior and
	the existing synchronized containers for queued delivery.
- Components should react to variant messages through focused handlers rather
	than one monolithic callback.
- For bounded async observers, treat queue overflow as an observable event when
	losing messages could affect correctness or diagnosis.

## Header and Documentation Conventions

- Do not rely on implicit or transitive includes; include what a `.cpp` or
	`.hpp` directly uses.
- Favor forward declarations in headers when they do not obscure correctness or
	required type completeness.
- New headers and source files should follow the Doxygen file-header style used
	by existing files in `tools/`, including `@file`, `@brief`, `@author`, and
	`@date` tags where that style is applicable.
- Put API documentation in headers and avoid duplicating it in implementation
	files.
- Use block Doxygen comments for classes, public methods, enums, and non-obvious
	constraints. Include `@param`, `@return`, and `@tparam` where relevant.
- Do not use `///` Doxygen comments.

## Error Handling and Testing

- New classes and APIs should be exception-free unless there is a strong,
	explicit reason otherwise.
- Prefer `tools::expected` for result-based APIs and return structured error
	information instead of throwing where practical.
- Favor explicit result checks in callers and demonstrations.
- For new or changed behavior, add success-path and failure-path coverage in
	the project's available test or demonstration code. This repository currently
	uses executable demonstrations in `main.cpp`; use a dedicated test framework
	only if one is introduced deliberately.

## Preferred Contribution Pattern

1. Read the nearby reference implementation in `tools/` and relevant examples
	 in `main.cpp`.
2. Make the smallest localized change that satisfies the behavior.
3. Keep naming, formatting, ownership, and documentation consistent with this
	 file and the existing code.
4. Add or update focused validation for both normal and failure behavior when
	 the change can fail.
5. Run `format_main.sh` when applicable, then validate diagnostics and build the
	 desktop target.
6. Update `README.md` and examples when public behavior or API usage changes.

## Effective Modern C++ Additions

- Prefer explicit lambda captures over `[=]` or `[&]` in non-trivial code.
- Use `nullptr` instead of `0` or `NULL` for null pointer intent.
- Prefer `using` aliases over `typedef`.
- Use `= delete` for disallowed operations.
- Assume move operations may not be cheap or available on every target; preserve
	correctness when copies are required.
- Use `std::move` only when transferring from a named object is intentional.
