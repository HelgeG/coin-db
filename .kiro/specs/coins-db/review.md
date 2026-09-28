# Review — coins-db v1

Final review of the implementation against `requirements.md`. Status: **v1
complete.** The C++ side builds clean under `-Werror` (C++23) with **116 tests**
passing; the web SPA typechecks and builds.

## Requirements coverage

### Req 1 — Record a coin
- Country, denomination, face value + own currency, year/range, mint/mint mark,
  free-text notes, composition, weight, diameter — `Coin` + `coin` table; set via
  CLI `add`, REST `POST /coins`, and the web form. ✅
- Condition/grade recorded on **any** scale: numeric (Sheldon 1–70), the
  Norwegian scale (`0, 0/01, 01, 1+, 1, 1-, 2, 3`), or adjectival (VF/XF/MS…).
  Scale-aware validation; unknown scales accepted. ✅
- Value estimate (in the base currency) with a date; acquisition
  date/price(base currency)/source. ✅
- Missing required fields rejected with clear messages (`validate_coin`; REST 422
  with field/message list; CLI/web show them). ✅

### Req 2 — Reference material and images
- Reference links (URL + label); invalid URLs rejected. ✅
- Images copied into a managed store (atomic write, unique naming); served back
  over `GET /images/{id}/file`; removed with their files. ✅

### Req 3 — Search / filter / sort
- Filter by country, year/range, denomination, grade, metal, value range (base
  currency);
  sort by year/country/latest value/date added; free-text over notes, country,
  denomination, and reference-link labels. ✅
- Export the last executed search's matching results to CSV (same flattened,
  localized format as the full CSV export; id-ordered). ✅

### Req 4 — Value tracking and summaries
- Collection summary: headline total in the base currency from each coin's
  **latest** estimate,
  always shown. ✅
- Selectable predefined breakdown (by country, total value only, by decade, by
  grade, by metal), combined with the headline total; default `by_country`.
  Available in the CLI (`summary --type`) and web (selector persisted in
  `localStorage`). ✅
- Estimates are append-only history; latest is the current value. ✅

### Req 5 — Update and delete
- All coin fields updatable (`update` / `PUT`). ✅
- Delete confirmation in the CLI (prompt, `--yes` to skip) and web (confirm
  dialog); deleting a coin purges its image files and rows. The REST `DELETE` is
  immediate by design (confirmation is a UI concern). ✅

### Req 6 — Import / export
- Export to JSON (full graph, with a self-contained `lookups` section so it
  round-trips into a freshly-seeded database) and CSV. ✅
- CSV renders the encoded fields (country, denomination, composition, mint,
  currency, unit) as their **localized display name** in the active language
  (no code columns); face value is shown with its unit. Takes a `--lang` /
  `?lang=` parameter (default English). ✅
- CSV export can be scoped to the results of a search (`GET /export?format=csv`
  honors the `/coins` filter params); the whole-collection export is the
  no-filter case. ✅
- Import validates the whole graph first and runs in a single transaction, so a
  bad file changes nothing; the `lookups` section is resolved by code (reusing
  seeded rows) and coin references are remapped. ✅ (see limitation below)

### Req 8 — Encoded, localized field vocabularies
- Country, denomination, composition, mint, and currency are shared, localized
  `lookup_entry` rows referenced by id; `mint_mark` stays free text. ✅
- Codes: country ISO 3166-1 alpha-2 (ISO 3166-3 for historical states —
  Yugoslavia/Czechoslovakia/USSR seeded); currency ISO 4217 incl. historical
  (DEM seeded); others app-generated. Country + currency seeded with en/nb names
  (idempotent). ✅
- Currencies have localized units (krone/øre, dollar/cent, …); a coin records
  face value against an optional unit (major unit if unset) — "50 øre". Named
  pieces use the optional denomination (Speciedaler, Skilling, Sovereign). ✅
- Add-on-the-fly with case-insensitive per-language de-duplication; web combo
  box; CLI accepts code-or-name with `--lang`; lookup-aware search and
  `by_country`/`by_metal` summaries (grouped by entry, localized labels). ✅
- Display falls back lang → English → any → code. ✅

### Domain rules & interfaces
- Per-coin currency kept, never converted; estimates/acquisition in the
  user-selectable collection base currency (default EUR, no conversion);
  country + a year required; append-only estimates. ✅
- Two interfaces on one shared core: CLI and web app. ✅
- Non-goals respected: no multi-user, no marketplace, no price scraping. ✅

## Known limitations (v1)

- **Import is restore-oriented.** It preserves ids and inserts; importing into a
  database that already contains overlapping ids fails and rolls back (no
  corruption) rather than merging/updating. Upsert/merge is future work.
- **Image files vs. JSON.** JSON export carries image metadata (`stored_path`);
  the image files themselves travel with the `<data-dir>/images/` directory in a
  filesystem backup.
- **Validation is shape-level.** ISO 4217 codes, URLs, and ISO dates are checked
  for shape, not membership/reachability/calendar validity.
- **SPA end-to-end tests.** Verified via typecheck + production build and a
  documented manual smoke test; automated browser E2E (Playwright) is deferred.
- **REST API is unauthenticated and localhost-only** by design (single-user v1).
