# Design — coins-db

## Status

Requirements are the source of truth. Core stack decisions are now confirmed
(see below); remaining choices are noted as minor open items.

## Confirmed Decisions

- **Interfaces**: a **CLI** and a **web app**, both built on one shared core.
- **Backend language**: **C++23**.
- **Coin "currency"**: an attribute of the coin (its own denomination currency,
  e.g. NOK, USD, GBP). Stored per coin.
- **Value estimates**: always expressed in **EUR**. No per-estimate currency field.
- **Images**: **copied into a managed store folder**; the DB stores the managed
  relative path, not the original external path.
- **Database**: SQLite (single-file, portable, easy to back up).
- **Web frontend**: a **Vue 3 + Vite + TypeScript** SPA talking to the C++ REST
  API. Vite builds static assets that `coins_server` serves directly.

## Architecture

```
                 +---------------------------+
                 |   coins_core (C++ lib)    |
                 |  domain model, validation |
                 |  SQLite data access,      |
                 |  value/summary logic,     |
                 |  image store manager,     |
                 |  import/export            |
                 +-------------+-------------+
                               |
              +----------------+-----------------+
              |                                  |
   +----------v----------+          +------------v-----------+
   |  coins_cli (C++)    |          |  coins_server (C++)    |
   |  command-line UI    |          |  REST/HTTP API         |
   +---------------------+          +------------+-----------+
                                                 |
                                    +------------v-----------+
                                    |  web frontend (SPA)    |
                                    |  (tool-agnostic)       |
                                    +------------------------+
```

- `coins_core`: the single source of truth for all logic. No UI concerns.
- `coins_cli`: thin CLI wrapper over `coins_core`.
- `coins_server`: thin HTTP/REST wrapper over `coins_core`; serves JSON.
- Web frontend: static SPA that consumes the REST API.

### Design principles (C++23, SOLID)

The core follows SOLID with dependency inversion at its center:

- Domain/validation logic depends on **abstract interfaces**, not concrete SQLite.
- Key interfaces (defined in `core/include/coins/`), implementations injected:
  - `ICoinRepository` — persistence of coins and their relations.
  - `IImageStore` — copy-in / retrieve / delete of managed image files.
  - (optionally) an `IClock` to make timestamps testable.
- SQLite and the filesystem image store are just implementations of these
  interfaces, so logic is unit-testable with in-memory/fake implementations and the
  storage backend is swappable without touching business rules.
- Single Responsibility: distinct types for data access, validation, value/summary
  logic, image store, and import/export rather than one large class.
- See steering `tech.md` for the full C++23 / modern-C++ / SOLID standards.

## Proposed C++ Libraries

Kept minimal and mainstream; final selection in Phase 0.

- **Build**: CMake.
- **SQLite access**: SQLite C API directly, or a thin wrapper (e.g., SQLiteCpp).
- **HTTP server**: a header-only/lightweight server (e.g., cpp-httplib) for the API.
- **JSON**: nlohmann/json for serialization (API, import/export).
- **CLI parsing**: CLI11 (or similar).
- **Testing**: GoogleTest or Catch2.
- **Dependency management**: vcpkg or Conan (decide in Phase 0).

## Data Model

```
coin
  id                INTEGER PK
  country           TEXT      NOT NULL
  denomination      TEXT                 -- e.g. "50 Øre"
  face_value        REAL                 -- numeric face value
  coin_currency     TEXT                 -- ISO 4217 of the coin itself, e.g. "NOK"
  year_from         INTEGER   NOT NULL   -- single year: year_from == year_to
  year_to           INTEGER   NOT NULL
  mint              TEXT
  mint_mark         TEXT
  composition       TEXT                 -- e.g. "Silver .900"
  weight_g          REAL
  diameter_mm       REAL
  grade_scale       TEXT                 -- e.g. "Sheldon", "Norwegian", adjectival
  grade_numeric     INTEGER              -- numeric scales (e.g. Sheldon 65)
  grade_label       TEXT                 -- symbolic grades (e.g. "MS", "VF", "1+", "0/01")
  acquired_date     TEXT                 -- ISO 8601 date
  acquired_price_eur REAL                -- price paid, normalized to EUR
  acquired_source   TEXT
  notes             TEXT
  created_at        TEXT      NOT NULL
  updated_at        TEXT      NOT NULL

value_estimate            -- history retained; latest = most recent estimated_at
  id            INTEGER PK
  coin_id       INTEGER   FK -> coin(id) ON DELETE CASCADE
  amount_eur    REAL      NOT NULL       -- always EUR
  estimated_at  TEXT      NOT NULL       -- ISO 8601 date
  source        TEXT                     -- how the estimate was derived

reference_link
  id        INTEGER PK
  coin_id   INTEGER   FK -> coin(id) ON DELETE CASCADE
  label     TEXT      NOT NULL           -- e.g. "Numista"
  url       TEXT      NOT NULL

image
  id            INTEGER PK
  coin_id       INTEGER   FK -> coin(id) ON DELETE CASCADE
  kind          TEXT                     -- "obverse" | "reverse" | "detail"
  stored_path   TEXT      NOT NULL       -- path relative to the managed image store
  original_name TEXT                     -- original filename, for reference
  caption       TEXT
```

### Relationships

- `coin` 1—N `value_estimate` (estimate history; current value = latest `estimated_at`).
- `coin` 1—N `reference_link`.
- `coin` 1—N `image`.

### Indexes

- `coin(country)`, `coin(year_from, year_to)`, `coin(coin_currency)` for filtering.
- `value_estimate(coin_id, estimated_at)` for fetching the latest estimate.

## Image Store

- A configured root directory, e.g. `<data-dir>/images/`.
- On attach, `coins_core` copies the source file into the store under a generated
  unique name (e.g. `<coin_id>/<uuid>.<ext>`) and records `stored_path` + `original_name`.
- On delete of a coin/image, the managed file is removed.
- The store is included in backups; export references images by `stored_path`.
- **Row/file coordination**: `IImageStore` (filesystem impl: `FilesystemImageStore`)
  handles only the bytes — atomic copy-in (`temp file + rename`), unique
  `<coin_id>/<uuid>.<ext>` naming, and delete. `ImageService` composes the store
  with the `image` table so rows and files stay consistent: the file is copied in
  before its row is written and rolled back if the insert fails, and both are
  removed together. Deleting a coin calls `ImageService::purge_coin_images` first
  (files + rows), since the DB row cascade alone would orphan the files.

## Value Handling

- Coin face value keeps its own `coin_currency` (informational; not converted).
- All monetary estimates and `acquired_price_eur` are stored in **EUR**.
- The collection summary always reports the headline total estimated value in EUR
  (latest estimate per coin) and, alongside it, one selectable **predefined
  breakdown** (a combined view).

### Predefined summaries

The core exposes a fixed, extensible set of predefined summary types. Each type
maps to one parameterized aggregate query and yields an ordered list of
`(label, value)` buckets. The initial set:

| Type key       | Meaning                          | Grouping / value                                  |
|----------------|----------------------------------|---------------------------------------------------|
| `by_country`   | Coins by country (default)       | `COUNT(*)` grouped by `country`                   |
| `total_value`  | Total estimated value (headline) | headline EUR total; no additional buckets         |
| `by_decade`    | Coins by year / decade           | `COUNT(*)` grouped by `(year_from/10)*10` decade  |
| `by_grade`     | Coins by grade                   | `COUNT(*)` grouped by `grade_label` (or scale)    |
| `by_metal`     | Coins by metal / composition     | `COUNT(*)` grouped by `composition`               |

- Breakdown buckets are sorted by descending count, then label ascending
  (`total_value` has no buckets — only the headline figure).
- The set is modeled as an `enum class SummaryType` plus a registry, so adding a
  new predefined summary is additive (Open/Closed): new enum value + query, no
  change to callers.
- The **headline total** is always computed regardless of the selected type.
- **Selection persistence** is a UI concern, not stored in the core DB: the web
  app persists the chosen type in `localStorage` (like theme/language); the CLI
  takes the type as a per-invocation option and has no persisted state. The
  default, when unspecified, is `by_country`.

## Search / Filter / Sort

`ICoinRepository::search(const CoinQuery&)` builds one dynamic, fully
parameterized query (no string interpolation of values). Coins are LEFT JOINed
to their latest estimate (via a `ROW_NUMBER()` window) so value filters and
value sorting work off the current value.

- Filters (all optional, combined with AND): `country` and `grade_label` (exact,
  `COLLATE NOCASE`); `denomination` and `composition` (case-insensitive
  substring); `year_from`/`year_to` (range overlap against each coin's range);
  `min_value_eur`/`max_value_eur` (bound the latest estimate; coins without an
  estimate are excluded when a bound is set).
- Free text (`text`): case-insensitive substring across country, denomination,
  notes, and reference-link labels.
- Sort: `DateAdded` (created_at), `Year`, `Country`, or `ValueEur`
  (latest estimate; coins without one sort last), ascending or descending, with
  coin id as a stable tie-break.

## REST API (sketch)

| Method | Path                          | Purpose                          |
|--------|-------------------------------|----------------------------------|
| GET    | /coins                        | List/search/filter/sort          |
| POST   | /coins                        | Create coin                      |
| GET    | /coins/{id}                   | Get coin with relations          |
| PUT    | /coins/{id}                   | Update coin                      |
| DELETE | /coins/{id}                   | Delete coin                      |
| POST   | /coins/{id}/estimates         | Add EUR value estimate           |
| POST   | /coins/{id}/links             | Add reference link               |
| DELETE | /links/{id}                   | Remove reference link            |
| POST   | /coins/{id}/images            | Upload image (multipart)         |
| GET    | /images/{id}/file             | Serve image bytes                |
| DELETE | /images/{id}                  | Remove image                     |
| GET    | /summary                      | Totals + a selectable breakdown  |
|        |   ?type=by_country\|total_value | (default `by_country`)           |
|        |         \|by_decade\|by_grade\|by_metal |                          |
| GET    | /export                       | JSON export                      |
| POST   | /import                       | JSON import                      |

## CLI (sketch)

```
coins add       --country ... --year ... [--denomination ...] [--face-value ... --coin-currency ...] ...
coins list      [--country ...] [--year ...] [--min-eur ...] [--sort year|country|value|added]
coins show      <id>
coins update    <id> --field value ...
coins delete    <id>            # prompts for confirmation
coins estimate  <id> --eur <amount> [--date ...] [--source ...]
coins link      <id> --label ... --url ...
coins image     <id> --file <path> [--kind obverse|reverse|detail]
coins summary   [--type by_country|total_value|by_decade|by_grade|by_metal]   # default by_country
coins export    --out collection.json
coins import    --in collection.json
```

## Validation Rules

- `country` and at least one year are required; `year_from <= year_to`.
- `coin_currency` (when present) is an ISO 4217 code.
- `value_estimate.amount_eur` and `acquired_price_eur` are numeric EUR amounts.
- `reference_link.url` must parse as a valid URL.
- Grade is scale-aware. `grade_scale` is free text so any system can be recorded
  (numeric or symbolic); the core validates the scales it knows: **Sheldon**
  (`grade_numeric` within 1..70) and **Norwegian** (symbolic `grade_label` in
  `0, 0/01, 01, 1+, 1, 1-, 2, 3`). Other scales (e.g. the adjectival F/VF/XF/AU/MS
  system) are accepted as-is, so adding a controlled scale is additive.
- Image source file must exist and be a supported image type before copying.

## Error Handling

Decided in Phase 1, applied consistently:

- **Exceptions for the exceptional.** Infrastructure and programming failures —
  SQLite errors, constraint violations, API misuse — throw. The data layer
  raises `coins::db::DatabaseError` (a `std::runtime_error` carrying the SQLite
  result code) from `Database`/`Statement`.
- **Result types for the expected.** Domain validation failures (missing
  country, inverted year range, bad URL, out-of-range grade) are *expected*
  outcomes, not exceptions. They are reported with an `std::expected`-style
  result type introduced with the validation layer in Phase 2, so callers must
  handle them explicitly rather than via `catch`.

## Import / Export

`coins::export_json` / `import_json` / `export_csv` (see `collection_io.hpp`):

- **JSON export** serializes the full collection graph — each coin with its value
  estimates, reference links, and image rows (images referenced by `stored_path`;
  the image files are backed up separately by copying the store directory).
- **JSON import** validates every coin and child row first and, if anything is
  invalid, writes nothing and returns the aggregated errors (field paths like
  `coins[0].country`). Malformed JSON is reported as a validation error. On
  success, rows are inserted in a single transaction with their **original ids
  preserved**, so a restore reproduces the collection exactly and keeps image
  `stored_path`s (which embed the coin id) valid. A storage failure rolls back.
- **CSV export** is a flattened, one-row-per-coin view (coin columns plus the
  latest EUR estimate and its date), with RFC-4180-style quoting.

## Security / Safety

- Single-user, local. No auth in v1 (API binds to localhost by default).
- All DB access uses parameterized statements (no string interpolation).
- Import validates and sanitizes input, and runs inside a single transaction so a
  bad file cannot partially corrupt the DB.
- Image copy validates type/size and writes atomically (temp file + rename).

## Testing Strategy

- Unit tests (GoogleTest/Catch2) for validation, value/summary logic, image store.
- Integration tests against a temporary SQLite DB for CRUD, search, export/import.
- Round-trip test: export → import into a fresh DB → data + images match.
- API tests hitting `coins_server` with an ephemeral DB and temp image store.

## Minor Open Items

- None. All minor open items are resolved (see below).

## Resolved Decisions

- **C++ dependency manager**: Conan 2.
- **Web frontend framework**: Vue 3 + Vite + TypeScript; Vite's static build is
  served by `coins_server`.
- **Acquisition price**: stored in **EUR only** (`acquired_price_eur`). The original
  paid currency is not retained; callers convert to EUR before recording.
