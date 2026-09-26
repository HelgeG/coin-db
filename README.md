# coins-db

A database and application for cataloging a personal physical coin collection.
Records rich per-coin detail (country, denomination, year, mint, composition,
condition/grade), tracks value estimates in EUR over time, keeps reference links
and images, and supports search, summaries, and import/export.

Delivered as a **CLI** and a **web app**, both built on a shared **C++23** core.

## Components

- `core/` — `coins_core`: shared library with all domain logic (no UI concerns).
- `cli/` — `coins_cli`: command-line interface.
- `server/` — `coins_server`: REST/HTTP API for the web frontend.
- `tests/` — GoogleTest-based test suite (unit + REST integration).
- `web/` — `coins-web`: Vue 3 + Vite + TypeScript SPA over the REST API.
- `samples/` — an importable example collection for a quick demo.

See `.kiro/specs/coins-db/` for requirements, design, and tasks, and
`.kiro/steering/` for product, tech, and structure guidance.

## Prerequisites

- A C++23 compiler (Apple clang 21+ / recent Clang or GCC)
- [CMake](https://cmake.org/) 3.24+
- [Ninja](https://ninja-build.org/)
- [Conan 2](https://conan.io/) for dependency management
- `clang-format` and `clang-tidy` (for formatting and linting)

Optional, only for the Kiro MCP servers configured in
`.kiro/settings/mcp.json`:

- [`uv`](https://docs.astral.sh/uv/) (provides `uvx`) — used to run the
  `sqlite` MCP server (`brew install uv`). The `sqlite` server points at
  `demo/coins.db`.
- Node.js / `npx` — used to run the `context7` MCP server. If it fails to
  start with a missing-module error, clear the stale npx cache
  (`rm -rf ~/.npm/_npx/*`) and retry.

## Build

Dependencies are provided by Conan, which also generates a CMake preset
(`conan-debug`) wired to the Conan toolchain.

```bash
# 1. Install dependencies + generate the toolchain and preset (C++23, Debug)
conan install . --output-folder=build --build=missing \
  -s build_type=Debug -s compiler.cppstd=23

# 2. Configure and build via the generated preset
cmake --preset conan-debug
cmake --build --preset conan-debug
```

The build tree is created under `build/build/Debug/` (Conan's `cmake_layout`).

## Test

The suite uses GoogleTest and is run via CTest. Testing is extensive: every core
module is expected to have unit tests.

```bash
ctest --preset conan-debug --output-on-failure
# or, explicitly:
ctest --test-dir build/build/Debug --output-on-failure
```

Disable tests (e.g. for a lean release build) with `-DCOINS_BUILD_TESTS=OFF`.

## Run

```bash
CLI=./build/build/Debug/cli/coins_cli
$CLI --help                                  # command-line interface
$CLI --data-dir mydata add --country Norway --year 1963 --denomination "50 Øre" \
     --coin-currency NOK --grade-scale Norwegian --grade-label 1+
$CLI --data-dir mydata estimate 1 --eur 120.50
$CLI --data-dir mydata list
$CLI --data-dir mydata summary
$CLI --data-dir mydata export --format json --out backup.json

# REST API on http://127.0.0.1:8080 (localhost only, no auth). Optional data dir arg.
./build/build/Debug/server/coins_server mydata
```

The CLI and server store the database and image store under the data directory
(CLI default `coins-data`): `<data-dir>/coins.db` and `<data-dir>/images/`.

### Sample data

Seed a data directory with an example collection (imports `samples/collection.sample.json`):

```bash
./scripts/seed.sh mydata          # uses ./build/build/Debug/cli/coins_cli
$CLI --data-dir mydata list
$CLI --data-dir mydata summary
```

### Web frontend

A Vue 3 + Vite + TypeScript SPA lives in `web/` and talks to the running
`coins_server`. In development Vite proxies `/api` to the server, so no CORS
setup is needed:

```bash
./build/build/Debug/server/coins_server mydata   # terminal 1
cd web && npm install && npm run dev              # terminal 2 -> http://localhost:5173
```

See `web/README.md` for details and a manual smoke test.

## Backup & restore

All state lives under the data directory, so a backup is a copy of that folder —
it contains both the SQLite database and the managed image files:

```bash
cp -R mydata mydata-backup            # full backup (DB + images)
```

For a portable, human-readable snapshot of the catalog, export to JSON (this
captures coins, value-estimate history, links, and image metadata; the image
*files* are the ones under `<data-dir>/images/`):

```bash
$CLI --data-dir mydata export --format json --out collection.json   # or --format csv
$CLI --data-dir fresh import --in collection.json                   # restore into a new dir
```

Import runs in a single transaction and preserves ids, so a restore reproduces
the collection exactly. To fully restore a collection that has images, import the
JSON and copy the `images/` directory across.

## Formatting & linting

- Formatting: `clang-format` with a Google-based style (see `.clang-format`).
  A git pre-commit hook (`.githooks/pre-commit`) formats staged C/C++ files.
  Enable it with `git config core.hooksPath .githooks`.
- Linting: `clang-tidy` (see `.clang-tidy`), using `compile_commands.json`
  exported into `build/`.
