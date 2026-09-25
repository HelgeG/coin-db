# Tech — coins-db

> Living document. Update as tools, versions, and conventions are decided.

## Stack

- **Backend language**: C++ — target **C++23** (modern C++, up to and including C++23).
- **Database**: SQLite (single-file, local, portable).
- **Interfaces**: CLI (`coins_cli`) and web app; a REST API server (`coins_server`)
  bridges the web frontend to the core.
- **Web frontend**: SPA consuming the REST API, built with **Vue 3 + Vite +
  TypeScript**. Vite produces static assets that `coins_server` serves directly
  (no separate Node runtime in production).

## Components

- `coins_core` — shared C++ library: domain model, validation, SQLite data access,
  value/summary logic, managed image store, import/export. No UI concerns.
- `coins_cli` — thin CLI over `coins_core`.
- `coins_server` — thin REST/HTTP layer over `coins_core`; serves JSON.
- web frontend — static SPA over the REST API.

## Proposed libraries (finalize in Phase 0)

- **Build**: CMake.
- **SQLite access**: SQLite C API or SQLiteCpp wrapper.
- **JSON**: nlohmann/json.
- **HTTP server**: cpp-httplib (or similar lightweight, header-only).
- **CLI parsing**: CLI11.
- **Testing**: GoogleTest or Catch2.
- **Dependency manager**: **Conan 2** (matches the team's workflow at work).

## Toolchain (verified on the dev machine)

- Conan 2.32, CMake 4.4.3, Ninja 1.13, clang-format 23.
- Compiler: Apple clang 21 is the default and is C++23-capable (its libc++ ships
  `std::expected`). Homebrew LLVM clang 23 (`/opt/homebrew/opt/llvm`) is also
  available as an alternative; the project does not require it.
- Conan generates the CMake toolchain + dependency config; CMake uses the Ninja
  generator.

## C++ engineering standards

- **Language standard**: target C++23; set `CMAKE_CXX_STANDARD 23` with
  `CMAKE_CXX_STANDARD_REQUIRED ON` and no compiler extensions.
- **Modern C++**: prefer standard-library and idiomatic constructs over hand-rolled
  or C-style code:
  - RAII for all resources; no raw `new`/`delete`. Use smart pointers
    (`std::unique_ptr` by default, `std::shared_ptr` only when ownership is shared).
  - Value semantics and move semantics where appropriate; follow the Rule of Zero
    (prefer no user-declared special members), or Rule of Five when a class manages
    a resource directly.
  - `std::optional`, `std::variant`, `std::string_view`, `std::span` where they fit.
  - Prefer `enum class` over unscoped enums; `constexpr`/`consteval` where possible.
  - Use modern C++20/23 features where they add clarity: concepts to constrain
    templates, ranges/views for transformations, `std::format` for string
    formatting, designated initializers, `<chrono>` for dates/times, and
    `std::expected` for expected-failure result types.
  - `const`-correctness everywhere; mark functions `noexcept` when they cannot throw.
  - Avoid macros; prefer `constexpr`, templates, and inline functions.
- **SOLID principles** — apply throughout:
  - **S**ingle Responsibility: each class/module has one reason to change
    (e.g., separate data access, validation, image store, and value logic).
  - **O**pen/Closed: extend behavior via new types/strategies, not by editing
    stable code; use interfaces for extension points.
  - **L**iskov Substitution: subtypes must honor their base-type contracts.
  - **I**nterface Segregation: prefer small, focused interfaces over broad ones.
  - **D**ependency Inversion: high-level logic depends on abstractions, not concrete
    implementations. In particular, `coins_core` logic depends on a storage
    *interface*, with SQLite as one implementation — this keeps logic testable and
    swappable.
- **Design implications**:
  - Define abstract interfaces (e.g., `ICoinRepository`, `IImageStore`) in
    `core/include/coins/`; inject implementations via constructors.
  - Domain/validation logic is independent of SQLite and of any UI.
  - Errors (decided in Phase 1): **exceptions for exceptional/infrastructure
    failures** (e.g. `coins::db::DatabaseError` for SQLite errors, constraint
    violations, API misuse) and **`std::expected`-style result types for expected
    domain validation failures** (introduced with the validation layer in
    Phase 2). Apply this split consistently.
- **Tooling**: enable high warning levels (`-Wall -Wextra -Wpedantic`, treat as
  errors in CI); apply `clang-format` and `clang-tidy` (decide config in Phase 0).

## Formatting

- **Style**: `clang-format` with the **Google C++ style as the base**, configured in
  `.clang-format` (with a 100-column limit and left pointer alignment as project
  overrides).
- **Requirement**: `clang-format` must be installed and on `PATH`
  (e.g. `brew install clang-format` on macOS). Hooks skip gracefully with a warning
  if it is missing.
- **Automatic formatting** happens two ways:
  1. **Kiro postToolUse hook** (`.kiro/hooks/format-cpp.sh`, wired in
     `.kiro/agents/coins.json`) — formats C/C++ files right after they're written
     during an agent session.
  2. **Git pre-commit hook** (`.githooks/pre-commit`) — formats and re-stages staged
     C/C++ files on commit. Enabled via `git config core.hooksPath .githooks`.
- Both hooks operate only on C/C++ files (`.c/.cc/.cpp/.cxx/.h/.hh/.hpp/.hxx`).

## Conventions

- All DB access uses parameterized statements — never string interpolation.
- Money: estimates and acquisition prices stored in EUR; coin face value keeps its
  own currency and is not converted.
- Dates stored as ISO 8601 strings (TEXT).
- Image writes are atomic (temp file + rename) into the managed store.
- Import runs inside a single transaction to avoid partial corruption.
- API binds to localhost by default (single-user, no auth in v1).

## Build & test

Conan provides dependencies and the CMake toolchain; CMake + Ninja build.

```bash
# 1. Install dependencies + generate toolchain and preset (C++23, Debug)
conan install . --output-folder=build --build=missing \
  -s build_type=Debug -s compiler.cppstd=23
# 2. Configure + build via the generated preset
cmake --preset conan-debug
cmake --build --preset conan-debug
# 3. Run tests
ctest --preset conan-debug --output-on-failure
```

The build tree lives under `build/build/Debug/` (Conan `cmake_layout`).
The default Conan profile uses `cppstd=gnu17`; pass `-s compiler.cppstd=23` (as
above) or set it in your profile so it matches the CMake C++23 requirement.

After any code change: build, then run relevant tests before presenting results.

## Open tech decisions

- None. Core stack decisions are finalized.

## Resolved tech decisions

- Web frontend framework: **Vue 3 + Vite + TypeScript** (static build served by
  `coins_server`).
- Acquisition price is stored in **EUR only**; the original paid currency is not
  retained (callers convert to EUR before recording).
