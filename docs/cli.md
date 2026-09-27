# coins-db CLI reference (`coins_cli`)

A complete guide to the command-line interface for cataloguing a coin
collection. The CLI is a thin wrapper over the shared `coins_core` library, so
its behaviour matches the web app and REST API.

## Contents

- [Invocation](#invocation)
- [Global options](#global-options)
- [Data directory and storage layout](#data-directory-and-storage-layout)
- [Controlled vocabularies and languages](#controlled-vocabularies-and-languages)
- [Commands](#commands)
  - [`add`](#add) · [`list`](#list) · [`show`](#show) · [`update`](#update) ·
    [`delete`](#delete) · [`estimate`](#estimate) · [`link`](#link) ·
    [`image`](#image) · [`lookups`](#lookups) · [`summary`](#summary) ·
    [`export`](#export) · [`import`](#import)
- [Exit codes](#exit-codes)
- [Worked example](#worked-example)

---

## Invocation

```
coins [GLOBAL OPTIONS] <command> [ARGS...]
```

The built binary lives at `./build/build/Debug/cli/coins_cli` after a debug
build. The examples below assume:

```bash
CLI=./build/build/Debug/cli/coins_cli
```

Every run requires exactly one command. Running with no command, or with
`--help`, prints usage. Any command also accepts `--help` for its own options
(e.g. `coins add --help`).

---

## Global options

These are given **before** the command name.

| Option | Default | Meaning |
|--------|---------|---------|
| `--data-dir <path>` | `coins-data` | Directory holding the database and image store. Created if missing. |
| `--lang <code>` | `en` | Active language for displaying and resolving vocabulary values (e.g. `en`, `nb`). |
| `--help`, `-h` | — | Print help and exit. |

Example:

```bash
$CLI --data-dir mydata --lang nb summary
```

`--lang` affects two things: how names are **displayed** (country, denomination,
currency, unit, and grouped summary labels are shown in that language, falling
back to English), and how free-text values you pass are **resolved or created**
(a new entry gets its name recorded in the active language). It does not change
stored ids or codes.

---

## Data directory and storage layout

All state lives under the data directory (default `coins-data`, or whatever
`--data-dir` points at):

```
<data-dir>/
├── coins.db          # SQLite database (coins, estimates, links, image rows, vocabularies)
└── images/           # managed image store; files named <coin_id>/<uuid>.<ext>
```

A backup is a copy of that directory. On first use the database is created and
its controlled vocabularies are **seeded** (see below).

---

## Controlled vocabularies and languages

Five coin fields are stored as shared, localised **lookup entries** rather than
free text, so the same real-world value is recorded consistently and shown in
your language:

- **country**, **denomination**, **composition** (metal), **mint**, and
  **currency**.

Each entry has a stable **code** and a display **name per language** (English
and Norwegian bokmål to start). `mint_mark` remains free text.

On any option that takes one of these fields you may pass **either a code or a
name**:

- If the text matches an existing entry's **code** (e.g. `NO`, `NOK`), that
  entry is used.
- Otherwise the text is matched against existing **names** in the active
  language, case-insensitively; a match is reused.
- If nothing matches, a **new entry is created** with the text as its name in
  the active language (and a generated or ISO code).

This "resolve-or-create" means you never have to pre-register a value — typing a
new country or denomination just works, and duplicates (same name, same
language, ignoring case) are merged automatically.

### Seeded data

A fresh database is pre-populated with:

- ISO 3166-1 countries **plus historical states** (e.g. Yugoslavia `YUCS`,
  Czechoslovakia `CSHH`, Soviet Union `SUHH`), with English and Norwegian names.
- ISO 4217 currencies **plus historical ones** (e.g. Deutsche Mark `DEM`), each
  with its standard **units** (e.g. NOK → krone + øre, USD → dollar + cent).

Use [`lookups`](#lookups) to browse what is available.

### Currency units and face value

A currency has one or more **units** so a coin's face value can be recorded
naturally — "50 øre" rather than "0.5 NOK". On `add`/`update`:

- `--face-value <number>` sets the numeric value.
- `--face-unit <code-or-name>` chooses the unit **within the coin's currency**
  (e.g. `øre`). It is only meaningful alongside `--currency`. If omitted, the
  value is interpreted in the currency's **major unit**.

Named historical pieces (Speciedaler, Skilling, Sovereign, …) are recorded with
`--denomination` instead of, or in addition to, a face value.

---

## Commands

### `add`

Add a coin. Only `--country` and a year are required.

```bash
coins add --country <code|name> --year <n> [options...]
```

| Option | Notes |
|--------|-------|
| `--country <code\|name>` | Country of origin. **Required.** |
| `--year <n>` | Year, or the start of a year range. |
| `--year-to <n>` | End of a year range (defaults to `--year` when omitted). |
| `--denomination <code\|name>` | Named piece (e.g. "50 Øre", "Speciedaler"). |
| `--face-value <number>` | Numeric face value. |
| `--currency <code\|name>` | The coin's own currency (e.g. `NOK`). |
| `--face-unit <code\|name>` | Unit for the face value (e.g. `øre`); major unit if omitted. |
| `--mint <code\|name>` | Minting facility. |
| `--mint-mark <text>` | Free-text mint mark. |
| `--composition <code\|name>` | Metal/alloy (e.g. "Silver .900"). |
| `--weight <grams>` | Weight in grams. |
| `--diameter <mm>` | Diameter in millimetres. |
| `--grade-scale <text>` | e.g. `Sheldon`, `Norwegian`, or any system. |
| `--grade-numeric <n>` | Numeric grade (validated 1–70 for Sheldon). |
| `--grade-label <text>` | Symbolic grade (e.g. `MS`, `1+`; validated for the Norwegian scale). |
| `--acquired-date <YYYY-MM-DD>` | Date acquired. |
| `--acquired-price-eur <number>` | Price paid, in EUR. |
| `--acquired-source <text>` | Where/how it was acquired. |
| `--notes <text>` | Free-text notes. |

Prints `Added coin <id>` on success. On invalid input it prints the validation
failures and exits non-zero (nothing is written).

```bash
$CLI --data-dir mydata add --country Norway --year 1963 \
     --denomination "50 Øre" --currency NOK --face-value 50 --face-unit øre \
     --mint Kongsberg --composition "Copper-nickel" \
     --grade-scale Norwegian --grade-label 1+ \
     --acquired-date 2024-11-03 --acquired-price-eur 2.5 --acquired-source "coin fair"
```

### `list`

List and search coins. With no filters, lists everything.

```bash
coins list [filters...] [--sort added|year|country|value] [--desc]
```

| Option | Notes |
|--------|-------|
| `--country <code\|name>` | Filter by country (matches the entry by name or code). |
| `--year-from <n>` / `--year-to <n>` | Range overlap against each coin's year range. |
| `--denomination <code\|name>` | Filter by denomination. |
| `--grade <label>` | Exact grade-label match. |
| `--metal <code\|name>` | Filter by composition/metal. |
| `--min-eur <n>` / `--max-eur <n>` | Bound the coin's latest EUR estimate. |
| `--text <text>` | Free-text search over localised names, notes, and link labels. |
| `--sort added\|year\|country\|value` | Sort field (default `added`). `country` sorts by localised name. |
| `--desc` | Sort descending (default ascending). |

Output is a table: `ID`, `Country`, `Year`, `Denomination`, `Latest EUR`,
followed by a count. Values are shown in the active language.

```bash
$CLI --data-dir mydata list --country Norway --sort value --desc
```

### `show`

Show one coin's full details.

```bash
coins show <id>
```

Prints every populated field (localised names for the vocabulary fields, face
value rendered with its unit, e.g. `50 øre`), the latest estimate and history
count, and any reference links and images. Exits non-zero if the id is unknown.

```bash
$CLI --data-dir mydata show 1
```

### `update`

Update fields of an existing coin. Takes the **same options as `add`**; only the
options you pass are changed, the rest are preserved.

```bash
coins update <id> [options...]
```

```bash
$CLI --data-dir mydata update 1 --grade-label 01 --notes "re-graded"
```

Prints `Updated coin <id>`; validation failures exit non-zero and change
nothing. Unknown id exits non-zero.

### `delete`

Delete a coin and its images (files and rows).

```bash
coins delete <id> [-y|--yes]
```

Prompts `Delete coin <id> and its images? [y/N]` and only proceeds on `y`/`Y`.
Pass `-y`/`--yes` to skip the prompt (useful in scripts). Unknown id exits
non-zero.

```bash
$CLI --data-dir mydata delete 3          # interactive
$CLI --data-dir mydata delete 3 --yes    # no prompt
```

### `estimate`

Append a EUR value estimate to a coin. Estimates are **append-only history**;
the most recent is the coin's current value.

```bash
coins estimate <id> --eur <amount> [--date YYYY-MM-DD] [--source <text>]
```

| Option | Notes |
|--------|-------|
| `--eur <amount>` | Estimated value in EUR. **Required.** |
| `--date <YYYY-MM-DD>` | Estimate date (defaults to today). |
| `--source <text>` | How the estimate was derived (e.g. "Numista range"). |

```bash
$CLI --data-dir mydata estimate 1 --eur 3.5 --date 2026-01-05 --source "re-check"
```

### `link`

Manage reference links (URL + label) on a coin. Requires a subcommand.

```bash
coins link add <coin_id> --label <text> --url <url>
coins link list <coin_id>
coins link rm <link_id>
```

- `add` — attach a link (`--label` and `--url` required); the URL must be a
  valid `http(s)` URL.
- `list` — print each link as `id  label  url`.
- `rm` — remove a link by its id.

```bash
$CLI --data-dir mydata link add 1 --label Numista --url https://numista.com/x
$CLI --data-dir mydata link list 1
$CLI --data-dir mydata link rm 4
```

### `image`

Manage images attached to a coin. Files are **copied into the managed store**
under the data directory. Requires a subcommand.

```bash
coins image add <coin_id> --file <path> [--kind obverse|reverse|detail] [--caption <text>]
coins image list <coin_id>
coins image rm <image_id>
```

- `add` — copy a source image into the store (`--file` required; `--kind` limited
  to `obverse`/`reverse`/`detail`).
- `list` — print each image as `id  stored_path`.
- `rm` — remove an image (file and row) by id.

```bash
$CLI --data-dir mydata image add 1 --file ~/photos/front.png --kind obverse
```

### `lookups`

List the entries of one controlled vocabulary, as `code<TAB>name` in the active
language. Useful for discovering codes/names before adding coins.

```bash
coins lookups <kind>
```

`<kind>` is one of `country`, `denomination`, `composition`, `mint`, `currency`.
An unknown kind exits non-zero.

```bash
$CLI --data-dir mydata --lang nb lookups country   # e.g. "NO<TAB>Norge"
$CLI --data-dir mydata lookups currency
```

### `summary`

Show collection totals: the headline coin count and total estimated EUR value,
plus one selectable breakdown.

```bash
coins summary [--type by_country|total_value|by_decade|by_grade|by_metal]
```

- Default `--type` is `by_country`.
- `total_value` shows only the headline figures (no breakdown list).
- `by_country` and `by_metal` group by the lookup entry and label buckets with
  localised names (honours `--lang`).

```bash
$CLI --data-dir mydata summary
$CLI --data-dir mydata --lang nb summary --type by_country
$CLI --data-dir mydata summary --type by_decade
```

### `export`

Export the whole collection.

```bash
coins export [--format json|csv] [--out <file>]
```

- `--format json` (default): a complete, portable snapshot including a
  self-contained vocabulary section, suitable for exact restore via `import`.
- `--format csv`: a flattened, one-row-per-coin spreadsheet view. Vocabulary
  fields are written as **localised names** in the active language (no codes),
  and the face value is shown with its unit.
- `--out <file>`: write to a file (prints `Exported to <file>`); otherwise the
  content goes to stdout.

```bash
$CLI --data-dir mydata export --format json --out backup.json
$CLI --data-dir mydata --lang nb export --format csv --out collection.nb.csv
$CLI --data-dir mydata export > backup.json          # stdout redirection
```

### `import`

Import a JSON collection produced by `export --format json`.

```bash
coins import --in <file>
```

The whole graph is validated first; if anything is invalid, **nothing is
written** and the failures are printed. On success it runs in a single
transaction and prints the counts imported. The vocabulary section is resolved
by code (reusing seeded entries), so a file exported elsewhere restores
correctly into a fresh database.

```bash
$CLI --data-dir fresh import --in backup.json
# Imported 12 coin(s), 20 estimate(s), 5 link(s), 3 image(s)
```

To fully restore a collection that has images, import the JSON and also copy the
source data directory's `images/` folder across.

---

## Exit codes

- `0` — success.
- Non-zero — an error occurred: a validation failure (printed as
  `error: validation failed` with per-field messages), an unknown id, an
  unreadable/unwritable file, an unknown lookup kind, or a CLI usage/parse error.

Errors and diagnostics are written to **stderr**; normal output goes to
**stdout**, so you can safely redirect data (e.g. `export > file`) without
capturing error text.

---

## Worked example

```bash
CLI=./build/build/Debug/cli/coins_cli
D=mydata

# Add a Norwegian coin recorded as "50 øre", with acquisition details.
$CLI --data-dir $D add --country Norway --year 1963 \
     --denomination "50 Øre" --currency NOK --face-value 50 --face-unit øre \
     --composition "Copper-nickel" --grade-scale Norwegian --grade-label 1+

# Track its value over time (append-only history).
$CLI --data-dir $D estimate 1 --eur 3.0 --date 2025-01-10 --source "Numista"
$CLI --data-dir $D estimate 1 --eur 3.5 --date 2026-01-05 --source "re-check"

# Add a reference link and an image.
$CLI --data-dir $D link add 1 --label Numista --url https://en.numista.com/x
$CLI --data-dir $D image add 1 --file ~/photos/front.png --kind obverse

# Browse and report (Norwegian display).
$CLI --data-dir $D --lang nb show 1
$CLI --data-dir $D --lang nb summary --type by_country

# Back up, then restore into a fresh directory.
$CLI --data-dir $D export --format json --out backup.json
$CLI --data-dir fresh import --in backup.json
```
