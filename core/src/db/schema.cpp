#include "coins/db/schema.hpp"

#include <string>

#include "coins/db/seed.hpp"
#include "coins/db/statement.hpp"

namespace coins::db {
namespace {

// Schema DDL (version 3). Mirrors design.md's data model. Notes:
//  - `id INTEGER PRIMARY KEY` aliases SQLite's rowid (auto-incrementing).
//  - Child tables cascade on parent deletion; foreign keys are enforced because
//    the connection sets `PRAGMA foreign_keys = ON`.
//  - Encoded coin fields (country, denomination, mint, composition, currency)
//    reference `lookup_entry`; the face value's unit references `currency_unit`.
//  - Value estimates and acquisition price are in the collection base currency
//    (see `app_setting.base_currency_id`); no conversion is performed.
//  - The coin table enforces the year-range invariant with a CHECK.
constexpr std::string_view kSchemaSql = R"sql(
CREATE TABLE IF NOT EXISTS lookup_entry (
  id   INTEGER PRIMARY KEY,
  kind TEXT NOT NULL,
  code TEXT NOT NULL,
  UNIQUE (kind, code)
);

CREATE TABLE IF NOT EXISTS lookup_name (
  id       INTEGER PRIMARY KEY,
  entry_id INTEGER NOT NULL REFERENCES lookup_entry(id) ON DELETE CASCADE,
  lang     TEXT NOT NULL,
  name     TEXT NOT NULL,
  UNIQUE (entry_id, lang)
);

CREATE TABLE IF NOT EXISTS currency_unit (
  id             INTEGER PRIMARY KEY,
  currency_id    INTEGER NOT NULL REFERENCES lookup_entry(id) ON DELETE CASCADE,
  code           TEXT NOT NULL,
  minor_per_unit INTEGER NOT NULL,
  is_major       INTEGER NOT NULL,
  UNIQUE (currency_id, code)
);

CREATE TABLE IF NOT EXISTS currency_unit_name (
  id      INTEGER PRIMARY KEY,
  unit_id INTEGER NOT NULL REFERENCES currency_unit(id) ON DELETE CASCADE,
  lang    TEXT NOT NULL,
  name    TEXT NOT NULL,
  UNIQUE (unit_id, lang)
);

CREATE TABLE IF NOT EXISTS coin (
  id                 INTEGER PRIMARY KEY,
  country_id         INTEGER NOT NULL REFERENCES lookup_entry(id),
  denomination_id    INTEGER REFERENCES lookup_entry(id),
  face_value         REAL,
  currency_id        INTEGER REFERENCES lookup_entry(id),
  face_unit_id       INTEGER REFERENCES currency_unit(id),
  year_from          INTEGER NOT NULL,
  year_to            INTEGER NOT NULL,
  mint_id            INTEGER REFERENCES lookup_entry(id),
  mint_mark          TEXT,
  composition_id     INTEGER REFERENCES lookup_entry(id),
  weight_g           REAL,
  diameter_mm        REAL,
  grade_scale        TEXT,
  grade_numeric      INTEGER,
  grade_label        TEXT,
  acquired_date      TEXT,
  acquired_price     REAL,
  acquired_source    TEXT,
  notes              TEXT,
  created_at         TEXT NOT NULL,
  updated_at         TEXT NOT NULL,
  CHECK (year_from <= year_to)
);

CREATE TABLE IF NOT EXISTS app_setting (
  key   TEXT PRIMARY KEY,
  value TEXT
);

CREATE TABLE IF NOT EXISTS value_estimate (
  id           INTEGER PRIMARY KEY,
  coin_id      INTEGER NOT NULL REFERENCES coin(id) ON DELETE CASCADE,
  amount       REAL    NOT NULL,
  estimated_at TEXT    NOT NULL,
  source       TEXT
);

CREATE TABLE IF NOT EXISTS reference_link (
  id      INTEGER PRIMARY KEY,
  coin_id INTEGER NOT NULL REFERENCES coin(id) ON DELETE CASCADE,
  label   TEXT    NOT NULL,
  url     TEXT    NOT NULL
);

CREATE TABLE IF NOT EXISTS image (
  id            INTEGER PRIMARY KEY,
  coin_id       INTEGER NOT NULL REFERENCES coin(id) ON DELETE CASCADE,
  kind          TEXT,
  stored_path   TEXT    NOT NULL,
  original_name TEXT,
  caption       TEXT
);

CREATE INDEX IF NOT EXISTS idx_coin_country ON coin(country_id);
CREATE INDEX IF NOT EXISTS idx_coin_denomination ON coin(denomination_id);
CREATE INDEX IF NOT EXISTS idx_coin_mint ON coin(mint_id);
CREATE INDEX IF NOT EXISTS idx_coin_composition ON coin(composition_id);
CREATE INDEX IF NOT EXISTS idx_coin_currency ON coin(currency_id);
CREATE INDEX IF NOT EXISTS idx_coin_years ON coin(year_from, year_to);
CREATE INDEX IF NOT EXISTS idx_value_estimate_coin ON value_estimate(coin_id, estimated_at);
CREATE INDEX IF NOT EXISTS idx_reference_link_coin ON reference_link(coin_id);
CREATE INDEX IF NOT EXISTS idx_image_coin ON image(coin_id);
CREATE INDEX IF NOT EXISTS idx_lookup_name_entry ON lookup_name(entry_id);
CREATE INDEX IF NOT EXISTS idx_lookup_name_resolve ON lookup_name(lang, name COLLATE NOCASE);
CREATE INDEX IF NOT EXISTS idx_currency_unit_currency ON currency_unit(currency_id);
)sql";

}  // namespace

std::string_view schema_sql() noexcept { return kSchemaSql; }

void bootstrap_schema(Database& db) {
  db.execute("BEGIN;");
  try {
    db.execute(kSchemaSql);
    // Seed the controlled vocabularies (idempotent, keyed by code) inside the
    // same transaction so a fresh DB comes up fully populated.
    seed_lookups(db);
    // Default the collection base currency to EUR if not already set.
    seed_base_currency(db);
    // PRAGMA does not accept bound parameters, and kSchemaVersion is a trusted
    // compile-time constant, so formatting it into the statement is safe.
    db.execute("PRAGMA user_version = " + std::to_string(kSchemaVersion) + ";");
  } catch (...) {
    db.execute("ROLLBACK;");
    throw;
  }
  db.execute("COMMIT;");
}

int read_schema_version(Database& db) {
  Statement stmt = db.prepare("PRAGMA user_version;");
  if (!stmt.step()) {
    return 0;
  }
  return static_cast<int>(stmt.column_int64(0));
}

}  // namespace coins::db
