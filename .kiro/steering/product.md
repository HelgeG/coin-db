# Product — coins-db

## What it is

`coins-db` is a personal application for cataloging a physical coin collection.
It stores detailed information about each coin, tracks condition and value over
time, keeps reference material and images, and lets the owner search and filter
the collection.

## Who it's for

A single collector (the owner) managing their own collection locally. It is not
a multi-user or marketplace product.

## Why

To replace ad-hoc notes/spreadsheets with a structured, searchable catalog that:
- captures rich per-coin detail (origin, denomination, year, mint, composition,
  physical measurements, condition/grade),
- tracks estimated value over time (in EUR) and acquisition details,
- keeps images and external reference links organized alongside each coin,
- reports collection totals and supports backup via export/import.

## Core capabilities

- Record coins with required country + year and many optional attributes.
- Attach reference links (URL + label) and images (copied into a managed store).
- Add EUR value estimates as a retained history (never overwritten).
- Search, filter, and sort the collection.
- View collection summary: total estimated EUR value plus a face-value breakdown
  by the coins' own currencies.
- Export/import for backup and portability.

## Interfaces

- A command-line interface (CLI).
- A web application. The web app additionally offers a Settings view for
  appearance (light/dark/system theme) and language (localization; English and
  Norwegian bokmål to start).

Both are built on one shared core so behavior is consistent across interfaces.

## Explicit non-goals (v1)

- Multi-user accounts or sharing.
- Buying/selling or marketplace integration.
- Automatic price scraping from external sources.

## Key domain rules

- Each coin has its own denomination currency (e.g. NOK, USD); face values are
  not converted between currencies.
- All value estimates and acquisition prices are expressed in EUR.
- Country and at least one year are required for every coin.
- Value estimates are append-only history; the latest is the current value.
- Coin grading is not tied to a single system: numeric scales (e.g. Sheldon) and
  symbolic scales (e.g. the Norwegian 0–3 scale, or adjectival F/VF/XF/AU/MS) are
  all supported.
