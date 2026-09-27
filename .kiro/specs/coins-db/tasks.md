# Tasks — coins-db

Implementation plan for a **C++** core with a **CLI** and a **web app**.
Tasks are ordered; each references the requirements it satisfies. Value estimates
are in **EUR**; images are copied into a **managed store**.

## Phase 0 — Setup & minor decisions

- [x] Choose C++ dependency manager (vcpkg vs. Conan). → **Conan 2**.
- [x] Choose web frontend approach. → **Vue 3 + Vite + TypeScript**.
- [x] Set up CMake project with three targets: `coins_core` (lib), `coins_cli`,
      `coins_server`; plus a test target.
- [x] Configure C++23 (`CMAKE_CXX_STANDARD 23`, required, no extensions).
- [x] Add warnings-as-errors (`-Wall -Wextra -Wpedantic`) and clang-format /
      clang-tidy configs.
- [x] Wire dependencies: SQLite, nlohmann/json, cpp-httplib, CLI11, GoogleTest.
- [x] Configure CI-friendly build + test (Conan preset `conan-debug`; verified
      clean build + `ctest` pass).
- [x] Add property-based testing harness (RapidCheck + GoogleTest integration,
      `RC_GTEST_PROP`); demo property in `tests/core/version_property_test.cpp`.
      Real properties are added alongside each module in the phases below.

## Phase 1 — `coins_core`: data layer

- [x] Define domain types (Coin, ValueEstimate, ReferenceLink, Image + ImageKind).
- [x] Define SQLite schema (design.md): `coin`, `value_estimate`, `reference_link`,
      `image`, with indexes (+ FK cascade, year-range CHECK).
- [x] Implement schema bootstrap / migrations (idempotent; `PRAGMA user_version`).
- [x] Implement data-access layer using parameterized statements (RAII
      `Database`/`Statement` wrappers; `DatabaseError`). Repository CRUD over
      these primitives lands in Phase 2.
- [x] Unit tests against a temp SQLite DB (+ first RapidCheck property on
      `ImageKind`).

## Phase 2 — `coins_core`: coin CRUD & validation (Req 1, 5)

- [x] Create coin (country + year required, year_from <= year_to; ISO 4217 coin_currency).
      Validation via `validate_coin` returning `std::expected`; grade validation is
      scale-aware (Sheldon numeric, Norwegian symbolic, other scales accepted).
- [x] Read / list coins (`get`/`list`). Loading child relations (estimates/links/
      images) lands with those features in Phases 3–4.
- [x] Update coin (refreshes `updated_at`, preserves `created_at`).
- [x] Delete coin — DB cascade to estimates/links/images done; removal of stored
      image *files* integrates with the image store in Phase 3.
- [x] Unit tests for CRUD + validation errors (+ create/get round-trip property).
      Introduced `ICoinRepository`, `SqliteCoinRepository`, and `IClock`.

## Phase 3 — `coins_core`: references & image store (Req 2)

- [x] Add/remove reference links with URL validation (`is_valid_url`,
      `validate_reference_link`; `IReferenceLinkRepository` + SQLite impl).
- [x] Implement managed image store (`IImageStore` + `FilesystemImageStore`:
      atomic temp-file + rename, unique `<coin_id>/<uuid>.<ext>` naming, delete).
- [x] Add/remove images (kind: obverse/reverse/detail), storing relative path
      (`ImageService` coordinates rows + files, with rollback on insert failure).
- [x] Tests for link validation, reference-link repo, image store, image service.
- [x] Coin-delete file cleanup: `ImageService::purge_coin_images` (called before
      coin removal; DB row cascade handles the rest).

## Phase 4 — `coins_core`: value tracking (Req 4)

- [x] Add EUR value estimate (append to history, never overwrite) —
      `IValueEstimateRepository` + SQLite impl; `validate_value_estimate`
      (non-negative amount, ISO date).
- [x] Query latest estimate per coin (`latest_for_coin`; `estimated_at` then id
      tie-break) and full history (`list_for_coin`, chronological).
- [x] Collection summary: total EUR estimate (latest per coin via window
      function) + per-country coin counts (`SummaryService`).
- [x] Tests for estimate validation, append-only history/latest, and summary math.

## Phase 5 — `coins_core`: search / filter / sort (Req 3)

- [x] Filter by country, year/range, denomination, grade, metal, EUR value range
      (`CoinQuery` + `ICoinRepository::search`, dynamic parameterized SQL with a
      latest-estimate LEFT JOIN).
- [x] Sort by year, country, latest EUR estimate, date added (asc/desc, id tie-break).
- [x] Free-text search over notes, country, denomination, and reference labels.
- [x] Tests for search/filter/sort (each filter, value range, sort orders, free text).

## Phase 6 — `coins_core`: import / export (Req 6)

- [x] JSON export of full collection graph (`export_json`; images referenced by
      stored_path).
- [x] JSON import inside a single transaction with validation (`import_json`;
      validate-all-first, ids preserved, aggregated field-path errors).
- [x] CSV export (flattened coin + latest EUR estimate; RFC-4180 quoting).
- [x] Round-trip test: export → import into fresh DB → re-export is byte-identical.
      (Image *files* are backed up by copying the store dir, not via JSON.)

## Phase 7 — `coins_cli`

- [x] Add `CollectionService` facade in `coins_core` (owns db + image store +
      clock; wires repositories/services; orchestrates coin delete + image purge).
- [x] Implement CLI commands over the facade (add, list, show, update, delete,
      estimate, link add/list/rm, image add/list/rm, summary, export, import).
- [x] Confirmation prompt on delete (stream-injectable; `--yes` to skip).
- [x] CLI-level tests for primary flows (add/list/show, validation failure,
      delete confirm/abort/--yes, estimate+summary, export→import) + facade tests.
      CLI logic lives in `coins_cli_lib` (thin `main`) so it is directly testable.

## Phase 8 — `coins_server` (REST API)

- [x] Implement REST endpoints (design.md API sketch) over the `CollectionService`
      (`server_app.cpp::register_routes` on cpp-httplib).
- [x] JSON request/response mapping (shared `coins/json.hpp`); multipart image
      upload (temp file → `add_image` → cleanup); 422 on validation, 404 on
      missing, 400 on bad JSON, 500 via exception handler.
- [x] Bind to localhost by default (main binds 127.0.0.1; prints a no-auth warning).
- [x] API integration tests with ephemeral DB + temp store (real httplib client
      on an ephemeral port). Server logic lives in `coins_server_lib` (thin `main`).

## Phase 9 — Web frontend (Vue 3 + Vite + TypeScript)

- [x] Scaffold the Vue 3 + Vite + TypeScript SPA consuming the REST API
      (`web/`; Vite dev proxy `/api` -> `coins_server`; typed API client).
- [x] Views: list/search, coin detail (with images + links + estimate history),
      add/edit forms (inline validation errors), value estimate entry, summary.
- [x] Settings view + appearance/localization: light/dark/system theme
      (`useTheme`, `data-theme` + CSS variables), selectable accent color
      (`useAccent`, `data-accent`; Violet or British Racing Green), and a
      hand-rolled i18n layer (`useI18n`, en/nb dictionaries) with a language
      selector. Nav and Settings are translated; remaining views translate
      incrementally.
- [~] End-to-end smoke test of primary flows: verified via `vue-tsc` typecheck +
      production `vite build`, plus a documented manual smoke test in
      `web/README.md`. Automated browser E2E (Playwright) deferred — no browser
      in the build environment.

## Phase 10 — Polish

- [x] README: build (CMake + Conan), run CLI/server/web, backup & restore
      (copy the data dir; JSON/CSV export/import).
- [x] Seed/sample data for demo (`samples/collection.sample.json` + `scripts/seed.sh`).
- [x] Final review against all requirements (`review.md`): v1 complete, 116 C++
      tests + SPA build green; known limitations documented.

## Phase 11 — Selectable predefined summaries (Req 4)

Extends the summary from a single fixed breakdown to a headline total plus one
selectable predefined breakdown, available in both the CLI and web app.

- [ ] Core: add `enum class SummaryType { ByCountry, TotalValue, ByDecade,
      ByGrade, ByMetal }` and a generic breakdown result (ordered
      `(label, count)` buckets). Extend `SummaryService` to compute the headline
      total plus the buckets for a requested type via parameterized aggregate
      queries (group by country / decade `(year_from/10)*10` / grade / metal).
- [ ] Core tests: one grouping test per `SummaryType` (incl. bucket ordering and
      empty-collection behavior) and a default-type test.
- [ ] Server: `GET /summary?type=...` (default `by_country`); serialize the
      headline total plus the selected breakdown buckets. Add API tests per type
      and for an unknown/invalid type (fall back to default or 400 — decide in
      implementation).
- [ ] CLI: `coins summary [--type by_country|total_value|by_decade|by_grade|by_metal]`
      (default `by_country`); print the headline total plus the selected
      breakdown. CLI tests for the default and at least one alternate type.
- [ ] Web: summary-type selector on the Summary view; persist the choice in
      `localStorage` (like theme/language), default `by_country`. Render the
      headline total plus the chosen breakdown table. Add i18n keys (en/nb) for
      the type labels.
- [ ] Update `postman/coins-db.postman_collection.json` (summary request with a
      `type` query param) and refresh `review.md`.
