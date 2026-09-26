# Requirements — coins-db

## Overview

`coins-db` is a database and application for cataloging a personal physical coin
collection. It lets the owner record detailed information about each coin, track
condition and value estimates, attach reference material and images, and search
and filter the collection.

It is delivered as a **CLI** and a **web app**, both built on a shared **C++**
core. Each coin records its own denomination currency (e.g. NOK, USD), while all
**value estimates are expressed in EUR**.

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
- WHEN I create a coin, THEN I can record a value estimate **in EUR** with the date
  the estimate was made.
- WHEN I create a coin, THEN I can record acquisition info: date acquired, price paid
  (in EUR), and source/seller.
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
  condition/grade, metal, and value range.
- WHEN I view results, THEN I can sort by year, country, value estimate, or date added.
- WHEN I search by free text, THEN it matches description, country, and reference labels.

### 4. Value tracking

As a collector, I want to know the total value of my collection.

- WHEN I view the collection summary, THEN I see the total estimated value in EUR
  (using the latest estimate per coin), and a face-value breakdown grouped by the
  coins' own currencies (face values are not converted).
- WHEN I add a new value estimate to a coin, THEN prior estimates are retained as
  history so I can see how the estimate has changed over time.

### 5. Update and delete

As a collector, I want to maintain my records.

- WHEN I edit a coin, THEN all recorded fields can be updated.
- WHEN I delete a coin, THEN I am asked to confirm before it is removed.

### 6. Import / export

As a collector, I want to back up and move my data.

- WHEN I export, THEN the full collection is written to a portable format (e.g., CSV
  and/or JSON).
- WHEN I import a previously exported file, THEN coins are added/updated without
  corrupting existing data.

### 7. Appearance and language (web app)

As a collector using the web app, I want to control appearance and language.

- WHEN I open Settings, THEN I can choose a light, dark, or system-follow theme,
  and the choice persists across sessions.
- WHEN I open Settings, THEN I can choose the interface language (English and
  Norwegian bokmål to start), and the choice persists across sessions.
- WHERE a string is not yet translated, THEN the app falls back to English.

## Data Attributes (summary)

| Attribute            | Notes                                             |
|----------------------|---------------------------------------------------|
| Country of origin    | Required                                          |
| Currency/denomination| e.g., "50 Øre", "1 Dollar"                        |
| Face value + currency| Numeric value plus the coin's own currency code   |
| Year / year range    | Required; support single year or range            |
| Mint / mint mark     | Optional                                          |
| Composition (metal)  | Optional                                          |
| Weight, diameter     | Optional physical measurements                    |
| Condition / grade    | Any scale: numeric (Sheldon) or symbolic (Norwegian, adjectival) |
| Value estimate (EUR) | Amount in EUR + date; history retained            |
| Reference links      | Zero or more (URL + label)                        |
| Images               | Zero or more; copied into managed store           |
| Acquisition info     | Date, price paid (EUR), source                    |
| Notes                | Free text                                         |
| Timestamps           | Created / updated                                 |

## Open Questions

- None. All initial open questions are resolved (see below).

## Resolved

- C++ dependency manager: **Conan 2**.
- Web frontend framework: **Vue 3 + Vite + TypeScript**.
- Acquisition price is stored in **EUR only**; the original paid currency is not
  retained (callers convert to EUR before recording).
