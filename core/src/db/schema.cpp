#include "coins/db/schema.hpp"

#include <string>

#include "coins/db/statement.hpp"

namespace coins::db {
namespace {

// Schema DDL. Mirrors design.md's data model. Notes:
//  - `id INTEGER PRIMARY KEY` aliases SQLite's rowid (auto-incrementing).
//  - Child tables cascade on coin deletion; foreign keys are enforced because
//    the connection sets `PRAGMA foreign_keys = ON`.
//  - The coin table enforces the year-range invariant with a CHECK so the
//    database cannot hold `year_from > year_to` regardless of the caller.
constexpr std::string_view kSchemaSql = R"sql(
CREATE TABLE IF NOT EXISTS coin (
  id                 INTEGER PRIMARY KEY,
  country            TEXT    NOT NULL,
  denomination       TEXT,
  face_value         REAL,
  coin_currency      TEXT,
  year_from          INTEGER NOT NULL,
  year_to            INTEGER NOT NULL,
  mint               TEXT,
  mint_mark          TEXT,
  composition        TEXT,
  weight_g           REAL,
  diameter_mm        REAL,
  grade_scale        TEXT,
  grade_numeric      INTEGER,
  grade_label        TEXT,
  acquired_date      TEXT,
  acquired_price_eur REAL,
  acquired_source    TEXT,
  notes              TEXT,
  created_at         TEXT    NOT NULL,
  updated_at         TEXT    NOT NULL,
  CHECK (year_from <= year_to)
);

CREATE TABLE IF NOT EXISTS value_estimate (
  id           INTEGER PRIMARY KEY,
  coin_id      INTEGER NOT NULL REFERENCES coin(id) ON DELETE CASCADE,
  amount_eur   REAL    NOT NULL,
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

CREATE INDEX IF NOT EXISTS idx_coin_country ON coin(country);
CREATE INDEX IF NOT EXISTS idx_coin_years ON coin(year_from, year_to);
CREATE INDEX IF NOT EXISTS idx_coin_currency ON coin(coin_currency);
CREATE INDEX IF NOT EXISTS idx_value_estimate_coin ON value_estimate(coin_id, estimated_at);
CREATE INDEX IF NOT EXISTS idx_reference_link_coin ON reference_link(coin_id);
CREATE INDEX IF NOT EXISTS idx_image_coin ON image(coin_id);
)sql";

}  // namespace

std::string_view schema_sql() noexcept { return kSchemaSql; }

void bootstrap_schema(Database& db) {
  db.execute("BEGIN;");
  try {
    db.execute(kSchemaSql);
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
