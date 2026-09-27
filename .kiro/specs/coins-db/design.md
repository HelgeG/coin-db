# Design — coins-db

## Status

Requirements are the source of truth. Core stack decisions are now confirmed
(see below); remaining choices are noted as minor open items.

## Confirmed Decisions

- **Interfaces**: a **CLI** and a **web app**, both built on one shared core.
- **Backend language**: **C++23**.
- **Coin "currency"**: an attribute of the coin (its own denomination currency,
  e.g. NOK, USD, GBP). Stored per coin as a reference to a `currency` **lookup
  entry** (ISO 4217 code, localized name; current or historical).
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
lookup_entry              -- shared, localized controlled vocabulary
  id            INTEGER PK
  kind          TEXT    NOT NULL     -- "country" | "denomination" | "composition" | "mint" | "currency"
  code          TEXT    NOT NULL     -- ISO 3166-1 alpha-2 for country; app-generated slug otherwise
  -- UNIQUE(kind, code)

lookup_name               -- one display name per entry per language
  id            INTEGER PK
  entry_id      INTEGER NOT NULL REFERENCES lookup_entry(id) ON DELETE CASCADE
  lang          TEXT    NOT NULL     -- "en" | "nb"
  name          TEXT    NOT NULL
  -- UNIQUE(entry_id, lang)
  -- UNIQUE(kind-scoped) case-insensitive name dedup enforced in the service layer

currency_unit             -- a denomination unit of a currency (e.g. krone, øre)
  id             INTEGER PK
  currency_id    INTEGER NOT NULL REFERENCES lookup_entry(id) ON DELETE CASCADE  -- kind = currency
  code           TEXT    NOT NULL     -- app-generated, unique within the currency (e.g. "ore")
  minor_per_unit INTEGER NOT NULL     -- how many of the currency's base (minor) unit this equals;
                                      --   major unit = 100 when 1 major = 100 minor; minor unit = 1
  is_major       INTEGER NOT NULL     -- 1 for the primary/major unit, 0 otherwise
  -- UNIQUE(currency_id, code)

currency_unit_name        -- localized name of a currency unit, per language
  id       INTEGER PK
  unit_id  INTEGER NOT NULL REFERENCES currency_unit(id) ON DELETE CASCADE
  lang     TEXT    NOT NULL           -- "en" | "nb"
  name     TEXT    NOT NULL           -- e.g. "øre" / "öre", "krone" / "krone"
  -- UNIQUE(unit_id, lang)

coin
  id                INTEGER PK
  country_id        INTEGER NOT NULL REFERENCES lookup_entry(id)   -- kind = country
  denomination_id   INTEGER          REFERENCES lookup_entry(id)   -- kind = denomination (named piece, optional)
  face_value        REAL                 -- numeric face value, expressed in face_unit
  currency_id       INTEGER          REFERENCES lookup_entry(id)   -- kind = currency (coin's own; not converted)
  face_unit_id      INTEGER          REFERENCES currency_unit(id)  -- unit the face_value is in; NULL = currency's major unit
  year_from         INTEGER   NOT NULL   -- single year: year_from == year_to
  year_to           INTEGER   NOT NULL
  mint_id           INTEGER          REFERENCES lookup_entry(id)   -- kind = mint
  mint_mark         TEXT                 -- free text (not a lookup)
  composition_id    INTEGER          REFERENCES lookup_entry(id)   -- kind = composition
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
- `coin` N—1 `lookup_entry` for each of country (required), denomination, mint,
  composition, and currency (optional). `lookup_entry` 1—N `lookup_name` (one per
  language).
- `lookup_entry` (kind=currency) 1—N `currency_unit`; `currency_unit` 1—N
  `currency_unit_name` (one per language). A coin optionally references a
  `currency_unit` as its `face_unit_id` (the unit `face_value` is expressed in).

### Indexes

- `coin(country_id)`, `coin(denomination_id)`, `coin(mint_id)`,
  `coin(composition_id)`, `coin(currency_id)` for filtering/grouping by lookup;
  `coin(year_from, year_to)` as before.
- `value_estimate(coin_id, estimated_at)` for fetching the latest estimate.
- `lookup_entry(kind, code)` UNIQUE; `lookup_name(entry_id, lang)` UNIQUE;
  `lookup_name(lang, name COLLATE NOCASE)` to support case-insensitive
  name→entry resolution.
- `currency_unit(currency_id, code)` UNIQUE; `currency_unit_name(unit_id, lang)`
  UNIQUE.

## Controlled Vocabularies (lookups)

Country, denomination, composition, mint, and currency are stored as shared,
localized **lookup entries** instead of free text (Req 8). This deduplicates
repeated values and lets each be shown in the active language.

### Model

- A `lookup_entry` is `(kind, code)`; `kind` is one of `country`,
  `denomination`, `composition`, `mint`, `currency`. Modelled in C++ as
  `enum class LookupKind`.
- `code` is the **ISO 3166-1 alpha-2** code for current countries (e.g. `NO`).
  Historical/defunct states use their **ISO 3166-3** formerly-used code when one
  exists (e.g. `YUCS`, `CSHH`, `SUHH`), or an app-generated country code
  otherwise. **Currency** uses the **ISO 4217** code (e.g. `NOK`), including
  historical ISO 4217 codes (e.g. `DEM`), or an app-generated code for
  currencies that predate ISO 4217. Other kinds use an **app-generated slug**
  (e.g. a normalized-name slug with a numeric suffix on collision). Codes are
  stable and never localized. Denomination entries carry no era/currency
  coupling — any denomination may be paired with any coin.
- Each entry has zero or more `lookup_name` rows — one display `name` per `lang`
  (`en`, `nb`). English is the fallback language.

### Interface (SOLID / dependency inversion)

- `ILookupRepository` (in `core/include/coins/`) — CRUD over entries + names:
  `find_by_code`, `find_by_name(kind, lang, name)` (case-insensitive), `list(kind,
  lang)`, `create(kind, code, names)`, `set_name(entry, lang, name)`.
- `SqliteLookupRepository` is the concrete implementation; logic depends on the
  interface so it stays unit-testable with a fake.
- A `LookupService` composes the repository and owns the resolution rules below.

### Resolution & de-duplication

- **Display**: `display_name(entry, lang)` returns the name for `lang`, else the
  English name, else any available name, else the `code`.
- **Resolve-or-create** (used by the web "add new" path and the CLI name form):
  given `(kind, lang, text)`, if an entry already has a name equal to `text`
  (case-insensitive) in `lang`, reuse it; otherwise create a new entry (generated
  code, or ISO code when the caller supplies one for a country) with `text` as
  its `lang` name.
- **CLI accepts code or name**: a value that matches an existing `(kind, code)`
  resolves directly; otherwise it is treated as a display name and run through
  resolve-or-create in the active language.

### Seeding

- On a fresh DB (see Migration), the `country` vocabulary is seeded from a
  compiled-in table of ISO 3166-1 countries **plus common historical states**
  (ISO 3166-3 codes such as `YUCS`, `CSHH`, `SUHH`), and the `currency`
  vocabulary from a compiled-in ISO 4217 table **plus common historical
  currencies** (e.g. `DEM`), each with `en` and `nb` names. Seeded currencies
  also seed their standard **units** (e.g. NOK → krone + øre, USD → dollar +
  cent, EUR → euro + cent) with en/nb names and `minor_per_unit` ratios. Seeding
  is idempotent (keyed by `(kind, code)` and `(currency_id, unit code)`), so
  re-running does nothing.

### Currency units

A currency has one or more **units** so a coin's face value can be recorded in
the natural denomination — e.g. "50 øre" rather than "0.5 NOK" (Req 8).

- Each `currency_unit` belongs to a `currency` lookup entry, has an app-generated
  `code` (unique within the currency), a localized name per language, an
  `is_major` flag, and `minor_per_unit` — how many of the currency's smallest
  (minor) unit it represents. For NOK: `krone` has `minor_per_unit = 100` and
  `is_major = 1`; `øre` has `minor_per_unit = 1`.
- A coin records `face_value` plus an optional `face_unit_id`. When the unit is
  omitted, the value is interpreted in the currency's **major** unit. So `{
  face_value: 50, unit: øre }` and `{ face_value: 0.5, unit: krone }` are
  distinct-but-equivalent recordings, displayed as entered ("50 øre").
- Units are **managed under a currency**: `LookupService` exposes
  `list_units(currency)`, `resolve_or_create_unit(currency, lang, name)`, and
  seeding. Un-seeded/added currencies simply start with no units; the user adds
  them on the fly (web combo box / CLI name).
- **Named denominations** (Speciedaler, Skilling, Sovereign, Ducat, …) are
  recorded with the optional `denomination` lookup — a named piece that is not a
  plain number×unit. A coin may use a denomination name, a face value + unit, or
  both; none is required.
- Display: a coin's value renders as `face_value <unit name>` when a unit is
  present (localized), falling back to `face_value <currency name>` using the
  major unit, and the denomination name is shown alongside when set.

### Migration

The schema gains lookup tables and the coin table swaps its free-text columns
for `*_id` foreign keys. Because there is **no production data**, the migration
is a clean, breaking `user_version` bump:

- Bump `kSchemaVersion` to 2 and add a migration step in `bootstrap_schema`
  (which becomes version-aware: create-if-absent for v1, then apply v2).
- v2 creates `lookup_entry`/`lookup_name`/`currency_unit`/`currency_unit_name`,
  seeds countries and currencies (with standard currency units), and defines the
  coin table with `*_id` columns and `face_unit_id`. Any pre-existing free-text
  data is not migrated (none exists); this trade-off is accepted per the resolved
  decision in requirements.md.

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

- Coin face value keeps its own `currency` lookup entry and an optional currency
  **unit** (`face_unit_id`); it is recorded in the natural denomination (e.g. "50
  øre") and is never converted between currencies. When the unit is omitted the
  value is the currency's major unit.
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
  (`total_value` has no buckets — only the headline figure). For `by_country` and
  `by_metal` the grouping is by **lookup entry id** and the bucket label is the
  entry's localized display name (see Controlled Vocabularies), so the same
  entry is never split across localized spellings.
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

- Filters (all optional, combined with AND): `country`, `denomination`, and
  `composition` match a **lookup entry** — the query accepts an entry code (or
  id) and filters on the coin's `*_id`; `grade_label` is exact (`COLLATE
  NOCASE`); `year_from`/`year_to` (range overlap against each coin's range);
  `min_value_eur`/`max_value_eur` (bound the latest estimate; coins without an
  estimate are excluded when a bound is set).
- Free text (`text`): case-insensitive substring across the coin's localized
  lookup names (country/denomination joined via `lookup_name`), notes, and
  reference-link labels.
- Sort: `DateAdded` (created_at), `Year`, `Country` (by the country entry's
  localized name in the active language), or `ValueEur` (latest estimate; coins
  without one sort last), ascending or descending, with coin id as a stable
  tie-break.

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
| GET    | /lookups/{kind}               | List entries of a kind (localized) |
| POST   | /lookups/{kind}               | Create/resolve an entry (name or code) |
| GET    | /currencies/{id}/units        | List a currency's units (localized) |
| POST   | /currencies/{id}/units        | Create/resolve a unit (name)     |
| GET    | /export?format=json\|csv&lang= | Export (JSON codes; CSV localized) |
| POST   | /import                       | JSON import                      |

`{kind}` is one of `country`, `denomination`, `composition`, `mint`, `currency`.
Listing
returns `{ id, code, name }` where `name` is resolved for the request's language
(via an `Accept-Language`-style `?lang=` param, defaulting to `en`).

**Coin JSON (breaking change).** A coin's encoded fields are represented as
nested objects rather than strings: `country`, `denomination`, `mint`,
`composition`, and `currency` each serialize as `{ id, code, name }` (localized
`name`), and are `null` when unset (except required `country`). On
create/update, callers send either an existing `id`/`code` or a `{ name }` to
resolve-or-create; `mint_mark` stays free text. Face value serializes as
`face_value` (number) plus `face_unit` (`{ id, code, name }` for the currency
unit, or `null` to mean the currency's major unit); on create/update the unit is
sent by `id`/`code` or `{ name }` and resolved within the coin's currency.

## CLI (sketch)

```
coins add       --country ... --year ... [--denomination ...] [--mint ...] [--composition ...] [--currency ...] [--face-value ... [--face-unit ...]] ...
                # --country/--denomination/--mint/--composition/--currency accept a CODE or a NAME
                #   (a name resolves in the active language, creating an entry if new)
                # --face-unit is a currency unit (e.g. "øre"); resolved within --currency; omit for the major unit
                # --denomination is for named pieces (Speciedaler, Skilling, Sovereign, …)
coins list      [--country ...] [--year ...] [--min-eur ...] [--sort year|country|value|added]
coins show      <id>
coins update    <id> --field value ...
coins delete    <id>            # prompts for confirmation
coins estimate  <id> --eur <amount> [--date ...] [--source ...]
coins link      <id> --label ... --url ...
coins image     <id> --file <path> [--kind obverse|reverse|detail]
coins summary   [--type by_country|total_value|by_decade|by_grade|by_metal]   # default by_country
coins lookups   <kind> [--lang en|nb]         # list a vocabulary's entries
coins export    --out collection.json
coins import    --in collection.json
```

The active language for CLI resolution/display comes from a `--lang` global
option (default `en`).

## Validation Rules

- `country` and at least one year are required; `year_from <= year_to`.
  `country` must reference an existing `country` lookup entry; when present,
  `denomination`/`mint`/`composition`/`currency` must reference an entry of the
  matching kind. Resolve-or-create happens **before** validation in the service
  layer, so by the time a coin is validated its `*_id` fields point at real
  entries.
- A lookup entry requires a non-blank `code` and at least one non-blank display
  name; `kind` must be one of the five known kinds. Country codes are uppercase
  alphanumeric and accept the ISO 3166-1 alpha-2 (2-letter), ISO 3166-3
  (4-letter historical), and app-generated shapes; currency codes accept the ISO
  4217 (3-letter, current or historical) and app-generated shapes — the app does
  not reject a country or currency solely because it is not a current ISO code.
  Name de-duplication (case-insensitive per kind+language) is enforced by
  `LookupService` on create.
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

- **JSON export** serializes the full collection graph — a top-level `lookups`
  section (all entries with codes + per-language names) plus each coin with its
  value estimates, reference links, and image rows. Coins reference lookups by
  `code` so the export is self-contained and portable. (Images referenced by
  `stored_path`; the image files are backed up separately by copying the store
  directory.)
- **JSON import** first imports the `lookups` section (resolve-or-create by
  `(kind, code)`), then validates every coin and child row and, if anything is
  invalid, writes nothing and returns the aggregated errors (field paths like
  `coins[0].country`). Malformed JSON is reported as a validation error. On
  success, rows are inserted in a single transaction with their **original coin
  ids preserved**, so a restore reproduces the collection exactly and keeps image
  `stored_path`s (which embed the coin id) valid. A storage failure rolls back.
- **CSV export** is a flattened, one-row-per-coin, human-readable view (coin
  columns plus the latest EUR estimate and its date), with RFC-4180-style
  quoting. Encoded fields (country, denomination, composition, mint, currency,
  currency unit) are written as their **localized display name in the active
  language** — no code columns. The face value is written together with its unit
  name (e.g. `50 øre`). CSV takes an active-language parameter (the CLI `--lang`
  / the request `?lang=`), defaulting to English. CSV is a lossy, presentational
  snapshot; **JSON is the format for exact round-trips** (it carries codes and
  the full `lookups` section).

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
