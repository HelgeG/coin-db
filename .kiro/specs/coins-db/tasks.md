# Tasks — coins-db

Implementation plan for a **C++** core with a **CLI** and a **web app**.
Tasks are ordered; each references the requirements it satisfies. Value estimates
and acquisition prices are in the collection's **base currency** (a user-selectable
`currency` lookup reference, default EUR; no conversion); images are copied into a
**managed store**.

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

- [x] Core: added `enum class SummaryType { ByCountry, TotalValue, ByDecade,
      ByGrade, ByMetal }` (`summary_type.hpp`, with wire keys + parsing) and a
      generic breakdown result (`SummaryBucket`/`SummaryBreakdown`). Extended
      `SummaryService::summarize(type)` to compute the headline total plus the
      buckets for a requested type via parameterized aggregate queries (country /
      decade `(year_from/10)*10` / grade / metal); buckets sorted in C++ by
      descending count then label, with the null bucket last in a tie.
- [x] Core tests: one grouping test per `SummaryType` (incl. bucket ordering,
      null buckets), empty-collection behavior, `total_value` has no buckets, and
      a default-type test.
- [x] Server: `GET /summary?type=...` (default `by_country`; unknown type falls
      back to the default); serializes `breakdown: { type, buckets }` alongside
      the headline total. API tests per type + unknown-type fallback.
- [x] CLI: `coins summary [--type by_country|total_value|by_decade|by_grade|by_metal]`
      (default `by_country`, validated via `CLI::IsMember`); prints the headline
      total plus the selected breakdown (heading per type; none for
      `total_value`). CLI tests for default, `by_decade`, `total_value`, invalid.
- [x] Web: summary-type selector on the Summary view; choice persisted in
      `localStorage` (`coins.summaryType`, default `by_country`). Renders the
      headline total plus the chosen breakdown table (hidden for `total_value`).
      i18n keys (en/nb) for the breakdown label and each type.
- [x] Updated `postman/coins-db.postman_collection.json` (summary request with a
      `type` query param + breakdown assertions) and refreshed `review.md`.

## Phase 12 — Encoded, localized field vocabularies (Req 8)

Turns country, denomination, composition, and mint into shared, localized lookup
entries referenced by id. Breaking schema/API/export change with a DB migration
(no production data). Ordered so each layer builds on a tested one below it.

- [x] Core domain types: `enum class LookupKind { Country, Denomination,
      Composition, Mint, Currency }` (with wire keys) and `LookupEntry { id,
      kind, code, names: map<lang,name> }`. Update `Coin` to carry `country_id`
      (required) and optional `denomination_id`/`mint_id`/`composition_id`/
      `currency_id` (drop the free-text `country`/`denomination`/`mint`/
      `composition` and the `coin_currency` string; keep `mint_mark`).
- [x] Schema + migration: add `lookup_entry`/`lookup_name` tables and the coin
      `*_id` columns + indexes; bump `kSchemaVersion` to 2 and make
      `bootstrap_schema` version-aware (v1 create-if-absent, then v2). Schema
      tests for the migration and constraints.
- [x] Country + currency seed data: compiled-in ISO 3166-1 alpha-2 table **plus
      common historical states** (ISO 3166-3 codes, e.g. `YUCS`/`CSHH`/`SUHH`)
      and a compiled-in ISO 4217 currency table **plus common historical
      currencies** (e.g. `DEM`), all with en/nb names; idempotent seeding on
      bootstrap. Test that a fresh DB has the expected current + historical
      country and currency entries and re-bootstrapping does not duplicate them.
- [x] `ILookupRepository` + `SqliteLookupRepository`: `find_by_code`,
      `find_by_name(kind, lang, name)` (NOCASE), `list(kind, lang)`,
      `create(kind, code, names)`, `set_name`. Repository tests incl.
      case-insensitive resolution and uniqueness.
- [x] `LookupService`: `display_name(entry, lang)` fallback chain
      (lang→en→any→code), `resolve_or_create(kind, lang, text)`, code-slug
      generation for non-country kinds, and case-insensitive name de-dup. Unit
      tests for each rule.
- [x] Currency units: `currency_unit`/`currency_unit_name` tables (schema v2),
      `CurrencyUnit { id, currency_id, code, minor_per_unit, is_major, names }`,
      repository + `LookupService::list_units/resolve_or_create_unit`; seed
      standard units for seeded currencies (NOK→krone/øre, USD→dollar/cent,
      EUR→euro/cent, …). Tests for unit resolution, seeding, and major-unit
      default.
- [x] Coin validation + repository: validate `*_id` references by kind and
      `face_unit_id` belongs to the coin's currency; update `SqliteCoinRepository`
      reads/writes for the new columns (incl. `face_unit_id`). Wire
      resolve-or-create (entries + units) into `CollectionService` add/update so
      callers can pass a code, an id, or a name. Update/extend coin CRUD tests.
- [x] Search / summary: filter/sort/free-text over lookup ids + localized names;
      `by_country`/`by_metal` breakdowns group by entry id with localized labels.
      Update search + summary tests.
- [x] CLI: `--country/--denomination/--mint/--composition/--currency` accept
      code-or-name; `--face-unit` (resolved within `--currency`, major unit if
      omitted); global `--lang` (default en); `coins lookups <kind> [--lang]`;
      localized value display ("50 øre") in `show`/`list`/`summary`. CLI tests.
- [x] Server: `GET/POST /lookups/{kind}` and `GET/POST /currencies/{id}/units`
      (with `?lang=`); coin JSON serializes lookup fields as `{ id, code, name }`,
      face value as `face_value` + `face_unit`, and accepts id/code/`{name}` on
      create/update. API tests incl. resolve-or-create, units, localized names.
- [x] Import / export: add a top-level `lookups` section to JSON incl. currency
      units (coins reference by code + unit code) for exact round-trips; CSV
      export renders encoded fields as **localized display names** in the active
      language (no code columns) and a face value with unit name (e.g. `50 øre`),
      taking a `--lang`/`?lang=` parameter (default en). JSON round-trip test +
      a CSV localization test (en vs nb).
- [x] Web: typed lookup API client + a reusable combo-box component (dropdown of
      localized entries + free-text add) used for the five fields (country,
      denomination, composition, mint, currency) in the coin form, plus a
      face-value + unit control (unit combo scoped to the chosen currency);
      localized value display across list/detail/summary; i18n keys. Typecheck +
      build.
- [x] Update `postman/coins-db.postman_collection.json` (lookups + new coin JSON)
      and refresh `review.md`. Update `samples/collection.sample.json` to the new
      format and re-verify `scripts/seed.sh`.

## Phase 13 — Web UI localization hardening (Req 9)

Makes the hand-rolled web i18n complete, enforced, and correctly formatted
(no new runtime dependency). Data-value localization (Req 8) is unchanged.

- [x] C — Split dictionaries: move message strings into per-area modules under
      `web/src/i18n/messages/` (`app`, `nav`, `common`, `lookup`, `coin`,
      `summary`, `settings`) and compose them into `en`/`nb`; keep
      `availableLocales`. No behaviour change; typecheck + build stay green.
- [x] A1 — Typed keys + completeness guard: derive `MessageKey = keyof typeof en`,
      type `t(key: MessageKey, params?)`, and declare each other locale as
      `Record<MessageKey, string>` so a missing/extra translation is a `vue-tsc`
      compile error (the enforcement mechanism — no test runner exists yet, and
      the typed `Record` is a stronger, build-time guarantee). Keep the runtime
      English fallback as a last resort.
- [x] B — Formatting + plurals: add `n(value, options?)`, `d(dateISO, options?)`,
      and a EUR currency helper to `useI18n` using `Intl.*` keyed on the active
      locale; extend `t()` to accept `{ one, other }` values selected by a `count`
      param via `Intl.PluralRules`. Apply `n()`/`d()` to the EUR total in
      SummaryView and to amounts/dates/`€price` in CoinDetailView.
- [x] A2 — Sweep hardcoded strings: route all user-facing text in
      `CoinListView.vue` and `CoinDetailView.vue` through `t()` (titles, search
      form labels + option labels, table headers, buttons, placeholders,
      empty-state messages, the delete `confirm()` text, "Choose an image file
      first"); add the new keys to `en` + `nb`. Exempt accent names and locale
      self-labels.
- [x] Verify: `vue-tsc` typecheck (which now enforces key completeness) +
      `vite build` pass; a manual language-switch check shows no residual English
      in nb.

## Phase 14 — User-selectable collection base currency (Req 4 & 7)

Replace the hardcoded EUR assumption with a single, user-selectable collection
base currency (a `currency` lookup reference) for value estimates and
acquisition price. No conversion. Breaking schema v3 (no production data).

- [x] Schema v3 + settings: add `app_setting` table; rename
      `value_estimate.amount_eur` → `amount` and `coin.acquired_price_eur` →
      `acquired_price`; bump `kSchemaVersion` to 3 (version-aware bootstrap);
      seed `base_currency_id` to the EUR currency entry. Schema/migration tests.
- [x] Core `SettingsService`: `base_currency()` (defaults/seeds EUR) and
      `set_base_currency(id)` (validates the id is a `currency` entry); expose via
      `CollectionService` plus a helper to format an amount with the base
      currency. Update `ValueEstimate`/`Coin` domain fields and validation
      (`amount`, `acquired_price`) and the repositories/JSON. Unit tests.
- [x] Summary + search: headline total and value filters use the renamed `amount`
      column; display the base currency. Update tests.
- [x] CLI: `estimate --amount` (rename from `--eur`), `add/update
      --acquired-price` (rename from `--acquired-price-eur`); new `base-currency`
      command (show / `set <code|name>`); show/list/summary display amounts with
      the base-currency code/name. CLI tests.
- [x] Server: `GET/PUT /settings` (base currency as `{ id, code, name }`); coin
      and summary JSON amounts unchanged in shape but documented as base currency;
      estimate endpoint accepts `amount`. API tests.
- [x] Web: Settings base-currency selector (currency combo, notes no-conversion);
      display estimates/acquisition/summary in the base currency (fetch it once);
      rename form fields. i18n keys (en/nb). Typecheck + build.
- [x] Import/export: JSON carries the base-currency setting (by code) and applies
      it on import; CSV amount columns are in the base currency. Update
      `samples/collection.sample.json`, postman, `review.md`. Round-trip test.
- [x] Verify: full build + ctest + web build; seed + base-currency smoke test.

## Phase 15 — Export filtered Collection results to CSV (Req 3 & 6)

Let the Collection screen export the results of the last executed search to a
CSV file, using the existing flattened, localized one-row-per-coin format
(id-ordered). No new formatting rules — only the row selection is scoped.

- [x] Core: add a query-aware `export_csv(db, const CoinQuery&, lang)` overload
      that emits the same columns/escaping/localized names as today but only for
      coins matching the query (reuse the existing search path — do not duplicate
      the WHERE-clause builder); redefine the no-arg overload as the empty-query
      case so there is one row-emitting path. Add the matching
      `CollectionService::export_csv(const CoinQuery&, lang)`. Core test: a
      filtered export returns only matching, localized rows (id order).
- [x] Server: `GET /export?format=csv` builds a `CoinQuery` from the same params
      as `GET /coins` (reuse `query_from_request`), passes the request `lang`, and
      responds with `text/csv` + `Content-Disposition: attachment; filename=…`.
      No filter params keeps the whole-collection behavior. API test: filtered
      export returns only matching rows + the attachment header.
- [x] Web: `client.exportCsvUrl(params, lang)` (reuses the search `queryString`);
      an "Export CSV" button on the Collection view whose href mirrors the **last
      executed search** params (not unapplied form edits). i18n key
      `collection.exportCsv` (en + nb). Typecheck + build.
- [x] Verify: full C++ build + ctest; web typecheck + build. Refresh `review.md`.
