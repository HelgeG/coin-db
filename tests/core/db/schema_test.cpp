#include "coins/db/schema.hpp"

#include <gtest/gtest.h>

#include <string_view>

#include "coins/db/database.hpp"
#include "coins/db/database_error.hpp"
#include "coins/db/statement.hpp"
#include "coins/id.hpp"

namespace {

using coins::db::Database;
using coins::db::DatabaseError;
using coins::db::Statement;

// True if a schema object (table/index/...) of the given type and name exists.
bool has_object(Database& db, std::string_view type, std::string_view name) {
  Statement stmt = db.prepare("SELECT count(*) FROM sqlite_master WHERE type = ? AND name = ?;");
  stmt.bind(1, type);
  stmt.bind(2, name);
  EXPECT_TRUE(stmt.step());
  return stmt.column_int64(0) > 0;
}

// Returns the id of a seeded country lookup entry (any one), for use as a valid
// `coin.country_id` foreign key.
coins::Id any_country_id(Database& db) {
  Statement stmt = db.prepare("SELECT id FROM lookup_entry WHERE kind = 'country' LIMIT 1;");
  EXPECT_TRUE(stmt.step());
  return stmt.column_int64(0);
}

// Inserts a minimal valid coin and returns its id.
coins::Id insert_coin(Database& db, int year_from, int year_to) {
  const coins::Id country_id = any_country_id(db);
  Statement stmt = db.prepare(
      "INSERT INTO coin (country_id, year_from, year_to, created_at, updated_at) "
      "VALUES (?, ?, ?, ?, ?);");
  stmt.bind(1, country_id);
  stmt.bind(2, year_from);
  stmt.bind(3, year_to);
  stmt.bind(4, std::string_view{"2026-01-01T00:00:00Z"});
  stmt.bind(5, std::string_view{"2026-01-01T00:00:00Z"});
  EXPECT_FALSE(stmt.step());
  return db.last_insert_rowid();
}

TEST(SchemaTest, BootstrapCreatesAllTablesAndIndexes) {
  Database db = Database::in_memory();
  coins::db::bootstrap_schema(db);

  for (const auto* table : {"coin", "value_estimate", "reference_link", "image", "lookup_entry",
                            "lookup_name", "currency_unit", "currency_unit_name"}) {
    EXPECT_TRUE(has_object(db, "table", table)) << "missing table: " << table;
  }
  for (const auto* index :
       {"idx_coin_country", "idx_coin_denomination", "idx_coin_mint", "idx_coin_composition",
        "idx_coin_currency", "idx_coin_years", "idx_value_estimate_coin", "idx_reference_link_coin",
        "idx_image_coin", "idx_lookup_name_entry", "idx_lookup_name_resolve",
        "idx_currency_unit_currency"}) {
    EXPECT_TRUE(has_object(db, "index", index)) << "missing index: " << index;
  }
}

TEST(SchemaTest, RecordsSchemaVersion) {
  Database db = Database::in_memory();
  coins::db::bootstrap_schema(db);
  EXPECT_EQ(coins::db::read_schema_version(db), coins::db::kSchemaVersion);
  EXPECT_EQ(coins::db::kSchemaVersion, 2);
}

TEST(SchemaTest, BootstrapIsIdempotent) {
  Database db = Database::in_memory();
  coins::db::bootstrap_schema(db);
  EXPECT_NO_THROW(coins::db::bootstrap_schema(db));
  EXPECT_EQ(coins::db::read_schema_version(db), coins::db::kSchemaVersion);
}

TEST(SchemaTest, DeletingCoinCascadesToChildren) {
  Database db = Database::in_memory();
  coins::db::bootstrap_schema(db);
  const coins::Id coin_id = insert_coin(db, 1889, 1889);

  Statement est = db.prepare(
      "INSERT INTO value_estimate (coin_id, amount_eur, estimated_at) VALUES (?, ?, ?);");
  est.bind(1, coin_id);
  est.bind(2, 100.0);
  est.bind(3, std::string_view{"2026-01-01"});
  ASSERT_FALSE(est.step());

  db.execute("DELETE FROM coin;");

  Statement count = db.prepare("SELECT count(*) FROM value_estimate;");
  ASSERT_TRUE(count.step());
  EXPECT_EQ(count.column_int64(0), 0);  // cascaded away
}

TEST(SchemaTest, ForeignKeyViolationIsRejected) {
  Database db = Database::in_memory();
  coins::db::bootstrap_schema(db);

  Statement est = db.prepare(
      "INSERT INTO value_estimate (coin_id, amount_eur, estimated_at) VALUES (?, ?, ?);");
  est.bind(1, coins::Id{999});  // no such coin
  est.bind(2, 100.0);
  est.bind(3, std::string_view{"2026-01-01"});
  EXPECT_THROW((void)est.step(), DatabaseError);
}

TEST(SchemaTest, YearRangeCheckRejectsInvertedRange) {
  Database db = Database::in_memory();
  coins::db::bootstrap_schema(db);
  EXPECT_THROW(insert_coin(db, 1990, 1980), DatabaseError);
  EXPECT_NO_THROW(insert_coin(db, 1980, 1990));
}

}  // namespace
