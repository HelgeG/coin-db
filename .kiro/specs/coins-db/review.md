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
- EUR value estimate with a date; acquisition date/price(EUR)/source. ✅
- Missing required fields rejected with clear messages (`validate_coin`; REST 422
  with field/message list; CLI/web show them). ✅

### Req 2 — Reference material and images
- Reference links (URL + label); invalid URLs rejected. ✅
- Images copied into a managed store (atomic write, unique naming); served back
  over `GET /images/{id}/file`; removed with their files. ✅

### Req 3 — Search / filter / sort
- Filter by country, year/range, denomination, grade, metal, EUR value range;
  sort by year/country/latest value/date added; free-text over notes, country,
  denomination, and reference-link labels. ✅

### Req 4 — Value tracking
- Collection summary: total EUR from each coin's **latest** estimate, plus a
  per-country coin count (how many coins from each country). ✅
- Estimates are append-only history; latest is the current value. ✅

### Req 5 — Update and delete
- All coin fields updatable (`update` / `PUT`). ✅
- Delete confirmation in the CLI (prompt, `--yes` to skip) and web (confirm
  dialog); deleting a coin purges its image files and rows. The REST `DELETE` is
  immediate by design (confirmation is a UI concern). ✅

### Req 6 — Import / export
- Export to JSON (full graph) and CSV (flattened + latest EUR). ✅
- Import validates the whole graph first and runs in a single transaction, so a
  bad file changes nothing. ✅ (see limitation below)

### Domain rules & interfaces
- Per-coin currency kept, never converted; estimates/acquisition in EUR;
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
