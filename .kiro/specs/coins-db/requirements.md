# Requirements — coins-db

## Overview

`coins-db` is a database and application for cataloging a personal physical coin
collection. It lets the owner record detailed information about each coin, track
condition and value estimates, attach reference material and images, and search
and filter the collection.

It is delivered as a **CLI** and a **web app**, both built on a shared **C++**
core. Each coin records its own denomination currency (e.g. NOK, USD). Value
estimates and acquisition prices are expressed in a single, user-selectable
**collection base currency** (e.g. EUR or NOK), chosen once for the collection.

## Goals

- Store rich, structured information about each coin in the collection.
- Make it easy to find, filter, and sort coins (by country, year, value, etc.).
- Track how much the collection is worth and how that changes over time.
- Keep links to external reference material (catalog entries, auction results).

## Non-Goals (initial version)

- Multi-user accounts / sharing.
- Marketplace / buying and selling integration.
- Automatic price scraping from external sources (may be added later).

## User Stories & Acceptance Criteria

### 1. Record a coin

As a collector, I want to add a coin with detailed attributes so that I have a
complete catalog entry.

- WHEN I create a coin, THEN I can record: country of origin, currency/denomination,
  face value, year (or year range), mint / mint mark, and a free-text description.
- WHEN I create a coin, THEN I can record its physical attributes: metal/composition,
  weight, diameter.
- WHEN I create a coin, THEN I can record its condition using any recognized grading
  scale — numeric (Sheldon 1–70), the Norwegian scale (0, 0/01, 01, 1+, 1, 1-, 2, 3),
  or adjectival grades like VF, XF, MS. The grading scale is not fixed to one system.
- WHEN I create a coin, THEN I can record a value estimate **in the collection's
  base currency** with the date the estimate was made.
- WHEN I create a coin, THEN I can record acquisition info: date acquired, price
  paid (**in the collection's base currency**), and source/seller.
- IF a required field (e.g., country, year) is missing, THEN the system rejects the
  entry with a clear validation message.

### 2. Reference material and images

As a collector, I want to attach links and images to a coin so I can cross-reference
it and see it visually.

- WHEN I edit a coin, THEN I can add one or more external reference links (URL + label,
  e.g., "Numista", "PCGS", auction listing).
- WHEN I edit a coin, THEN I can attach one or more images (obverse, reverse, detail).
  Attached images are **copied into a managed store folder** and referenced from there.
- IF a link is not a valid URL, THEN the system rejects it with a validation message.

### 3. Search, filter, and sort

As a collector, I want to find coins quickly.

- WHEN I search, THEN I can filter by country, year (or range), denomination,
  condition/grade, metal, and value range. For the encoded fields (country,
  denomination, metal) the filter matches the **lookup entry**, and results
  display its localized name (see Req 8).
- WHEN I view results, THEN I can sort by year, country, value estimate, or date added.
- WHEN I search by free text, THEN it matches description, country, and reference labels.

### 4. Value tracking and summaries

As a collector, I want to know the total value of my collection and view it
broken down in different predefined ways.

- WHEN I view the collection summary, THEN I always see the headline total
  estimated value in the collection's **base currency** (using the latest
  estimate per coin).
- WHEN I view the collection summary, THEN I can choose one breakdown from a list
  of predefined summaries, shown together with the headline total (combined view).
- The initial set of predefined breakdowns is:
  - Coins by country (count per country).
  - Total estimated value in the base currency (headline figure).
  - Coins by year / decade.
  - Coins by grade.
  - Coins by metal / composition.
- WHEN I select a breakdown, THEN the selection is remembered and persists across
  sessions; where no selection has been made, a sensible default breakdown
  (coins by country) is shown.
- Selecting a predefined summary is available in **both the web app and the CLI**
  (e.g. the CLI accepts a summary-type option).
- WHEN I add a new value estimate to a coin, THEN prior estimates are retained as
  history so I can see how the estimate has changed over time.

### 5. Update and delete

As a collector, I want to maintain my records.

- WHEN I edit a coin, THEN all recorded fields can be updated.
- WHEN I delete a coin, THEN I am asked to confirm before it is removed.

### 6. Import / export

As a collector, I want to back up and move my data.

- WHEN I export, THEN the full collection is written to a portable format. Both
  **JSON** and **CSV** exports are supported.
- WHERE I export to CSV, THEN encoded fields (country, denomination, composition,
  mint, currency, and currency unit) are written as their **localized display
  name in the active language** — never as codes — so the CSV is a
  human-readable snapshot. (JSON export remains code-based and self-contained for
  exact round-trips.)
- WHEN I import a previously exported file, THEN coins are added/updated without
  corrupting existing data.

### 7. Appearance and language (web app)

As a collector using the web app, I want to control appearance and language.

- WHEN I open Settings, THEN I can choose a light, dark, or system-follow theme,
  and the choice persists across sessions.
- WHEN I open Settings, THEN I can choose the accent color used for buttons and
  links (e.g. Violet or British Racing Green), independent of the theme, and the
  choice persists across sessions.
- WHEN I open Settings, THEN I can choose the interface language (English and
  Norwegian bokmål to start), and the choice persists across sessions.
- WHEN I open Settings, THEN I can choose the collection's **base currency** (the
  currency that value estimates and acquisition prices are expressed in, e.g. EUR
  or NOK), selected from the currency vocabulary. It is a collection-wide setting
  stored with the collection (not a per-device preference), defaults to EUR, and
  amounts are shown in that currency across the CLI and web. No conversion is
  performed — amounts are the values entered.
- WHERE a string is not yet translated, THEN the app falls back to English.

### 8. Encoded, localized field vocabularies

As a collector, I want frequently-repeated coin fields stored as reusable,
localized values so the same real-world thing is recorded consistently and shown
in my language.

The fields covered by this (the "controlled vocabularies" / lookups) are:
**country, denomination, composition (metal), mint, and currency** (the coin's
own face-value currency). Each other coin field is unchanged.

- WHEN a coin records one of these fields, THEN it references a shared lookup
  **entry by code** rather than storing free text, so the same entry is reused
  across coins.
- Each lookup entry has a stable **code** and a **display name per interface
  language** (English and Norwegian bokmål to start). For **country** the code is
  the standard **ISO 3166-1 alpha-2** code for currently-existing countries (e.g.
  `NO`, `US`); for **currency** the code is the **ISO 4217** code (e.g. `NOK`);
  for denomination, composition, and mint the code is **app-generated**.
- WHERE a country no longer exists (historical states such as Yugoslavia,
  Czechoslovakia, or the Soviet Union), THEN I can still record it: it is a
  first-class country entry using its **ISO 3166-3** formerly-used code when one
  exists (e.g. `YUCS` Yugoslavia, `CSHH` Czechoslovakia, `SUHH` USSR), or an
  app-generated country code otherwise. Historical and current countries are
  treated identically everywhere (selection, search, summaries).
- WHERE a currency is no longer in use (e.g. the Deutsche Mark, or pre-euro
  national currencies), THEN I can still record it: it is a first-class currency
  entry using its **ISO 4217** historical code when one exists (e.g. `DEM`), or
  an app-generated currency code for currencies that predate ISO 4217. Denomination
  entries are **era-independent** — any denomination can be recorded for any
  coin, with no coupling to a country or to a currency's period of use.
- WHEN the app starts on a fresh database, THEN the country vocabulary is
  **seeded** with ISO 3166-1 countries and a set of common historical states
  (ISO 3166-3 formerly-used codes), and the currency vocabulary is **seeded**
  with ISO 4217 current and common historical currencies — each with English and
  Norwegian names. Seeded currencies also seed their standard **units** (e.g. NOK
  → krone + øre, USD → dollar + cent).
- WHERE a currency has more than one denomination unit (e.g. krone and øre,
  dollar and cent), THEN each currency has one or more localized **units** and I
  can record a coin's face value in a chosen unit — so a coin reads as "50 øre",
  not "0.5 NOK". Choosing a unit is **optional**; when omitted the value is in the
  currency's major unit. I can add a new unit to a currency on the fly (like other
  vocabulary entries).
- WHERE a coin is a named historical piece (Speciedaler, Skilling, Sovereign,
  Ducat, …), THEN I can record it via the optional **denomination** name; a coin
  may use a denomination name, a face value + unit, or both, and none is required.
- WHEN I view a coin (web or CLI), THEN each lookup field is shown using the
  entry's display name **in the active language**, falling back to English, then
  to any available name, then to the code.
- WHEN I add or edit a coin and a needed value does not yet exist, THEN I can add
  a new vocabulary entry freely; its display name is recorded **in the active
  language**.
- WHERE a new entry's display name matches an existing entry's name in the same
  language (compared **case-insensitively**), THEN the existing entry is reused
  rather than creating a duplicate.
- WHEN I use the web app, THEN each lookup field is presented as a **combo box**:
  a dropdown to pick an existing entry (shown by localized name) combined with a
  text field to type and add a new one.
- WHEN I use the CLI, THEN a lookup field accepts **either a code or a display
  name**; a name is resolved to an existing entry (case-insensitive, active
  language) or creates a new entry if none matches.
- WHEN I search or view a summary that groups by one of these fields, THEN
  grouping and filtering operate on the **entry** (so localized names of the same
  entry are never split), while results display the localized name.

### 9. Complete, correctly-formatted UI localization (web app)

As a collector using the web app, I want the whole interface — not just parts of
it — shown correctly in my chosen language.

- WHEN I switch language, THEN **all** user-facing interface text changes,
  including every view (collection list, coin detail, forms, summary, settings),
  table headers, form labels, buttons, placeholders, empty-state messages, and
  confirmation prompts. (Data values are localized separately via Req 8; proper
  names such as accent-color names and each language's own label are exempt.)
- WHERE a translation is missing for the active language, THEN the app falls back
  to English (never showing a raw key).
- The set of translatable strings is **enforced to be complete**: it is not
  possible to ship the app with a translation key that some supported language
  lacks, and referring to a non-existent key is caught before release (a
  build/type or test failure), not discovered at runtime.
- WHEN numbers, monetary amounts, and dates are shown, THEN they are formatted
  according to the active locale's conventions (e.g. decimal separators and date
  order), rather than a fixed format.
- WHERE a message depends on a count (e.g. "1 coin" vs "2 coins"), THEN the
  correct singular/plural form for the active language is shown.

## Data Attributes (summary)

| Attribute            | Notes                                             |
|----------------------|---------------------------------------------------|
| Country of origin    | Required; **lookup entry** (ISO 3166-1 alpha-2 code, localized name) |
| Denomination         | Optional; **lookup entry** (generated code, localized name); a named piece (e.g. Speciedaler, Skilling, Sovereign), era-independent |
| Face value           | Optional numeric value, expressed in a currency unit (e.g. 50 øre) |
| Currency             | Optional; **lookup entry** (ISO 4217 code, localized name); the coin's own face-value currency, never converted |
| Currency unit        | Optional; a localized unit of the currency (e.g. krone, øre); the unit the face value is in (major unit if unset) |
| Year / year range    | Required; support single year or range            |
| Mint                 | Optional; **lookup entry** (generated code, localized name) |
| Mint mark            | Optional free text                                |
| Composition (metal)  | Optional; **lookup entry** (generated code, localized name) |
| Weight, diameter     | Optional physical measurements                    |
| Condition / grade    | Any scale: numeric (Sheldon) or symbolic (Norwegian, adjectival) |
| Value estimate       | Amount in the base currency + date; history retained |
| Reference links      | Zero or more (URL + label)                        |
| Images               | Zero or more; copied into managed store           |
| Acquisition info     | Date, price paid (in the base currency), source   |
| Notes                | Free text                                         |
| Timestamps           | Created / updated                                 |

## Open Questions

- None. All initial open questions are resolved (see below).

## Resolved

- C++ dependency manager: **Conan 2**.
- Web frontend framework: **Vue 3 + Vite + TypeScript**.
- Value estimates and acquisition prices are expressed in a single,
  user-selectable **collection base currency** (a reference to a `currency`
  vocabulary entry), stored as a collection-wide setting and defaulting to EUR.
  **No conversion** is performed — amounts are stored exactly as entered and
  shown in the base currency; the original paid currency is not separately
  retained. (This supersedes the earlier "EUR only" decision. Because there is no
  production data, the schema/API/CLI change in a breaking way.)
- **Encoded localized vocabularies** (Req 8): covers country, denomination,
  composition, mint, and **currency**. Country uses ISO 3166-1 alpha-2 codes
  (ISO 3166-3 for historical states); currency uses ISO 4217 codes (current +
  historical); denomination/composition/mint use app-generated codes. All are
  seeded (country, currency) or grow on demand, addable freely and de-duplicated
  by case-insensitive name match within a language. Display names are stored per
  app language (en/nb) with English fallback. Because there is no production data
  yet, the coin schema/API/export formats change in a **breaking** way (with a DB
  migration), rather than preserving the old free-text shape. `coin_currency`
  (free ISO text) becomes `currency_id` referencing a currency lookup entry.
  Currencies have localized **units** (e.g. krone/øre) seeded for seeded
  currencies and addable on demand; a coin records its face value against an
  optional unit (major unit if unset), so "50 øre" is recorded as such. Named
  pieces (Speciedaler, Skilling, Sovereign) use the optional denomination name.
