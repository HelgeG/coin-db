#include "coins/db/database.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "coins/db/database_error.hpp"
#include "coins/db/statement.hpp"

namespace {

using coins::db::Database;
using coins::db::DatabaseError;
using coins::db::Statement;

// A fresh in-memory database with a simple table for exercising the wrapper.
Database make_db_with_table() {
  Database db = Database::in_memory();
  db.execute(
      "CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT NOT NULL, "
      "amount REAL, note TEXT);");
  return db;
}

TEST(DatabaseTest, PreparedInsertRoundTripsValues) {
  Database db = make_db_with_table();

  Statement insert = db.prepare("INSERT INTO t (name, amount, note) VALUES (?, ?, ?);");
  insert.bind(1, std::string_view{"1889-CC"});
  insert.bind(2, 42.5);
  insert.bind(3, std::string_view{"morgan dollar"});
  EXPECT_FALSE(insert.step());  // INSERT yields no rows

  const std::int64_t rowid = db.last_insert_rowid();
  EXPECT_GT(rowid, 0);

  Statement select = db.prepare("SELECT id, name, amount, note FROM t WHERE id = ?;");
  select.bind(1, rowid);
  ASSERT_TRUE(select.step());
  EXPECT_EQ(select.column_int64(0), rowid);
  EXPECT_EQ(select.column_text(1), "1889-CC");
  EXPECT_DOUBLE_EQ(select.column_double(2), 42.5);
  EXPECT_EQ(select.column_text(3), "morgan dollar");
  EXPECT_FALSE(select.step());  // exactly one row
}

TEST(DatabaseTest, BindsOptionalPresentAndAbsent) {
  Database db = make_db_with_table();

  Statement insert = db.prepare("INSERT INTO t (name, note) VALUES (?, ?);");
  const std::optional<std::string> present = "has-note";
  const std::optional<std::string> absent;
  insert.bind(1, std::string_view{"a"});
  insert.bind(2, present);
  ASSERT_FALSE(insert.step());

  insert.reset();
  insert.bind(1, std::string_view{"b"});
  insert.bind(2, absent);
  ASSERT_FALSE(insert.step());

  Statement select = db.prepare("SELECT note FROM t ORDER BY id;");
  ASSERT_TRUE(select.step());
  EXPECT_FALSE(select.is_null(0));
  EXPECT_EQ(select.column_opt_text(0), std::optional<std::string>{"has-note"});
  ASSERT_TRUE(select.step());
  EXPECT_TRUE(select.is_null(0));
  EXPECT_EQ(select.column_opt_text(0), std::nullopt);
}

TEST(DatabaseTest, PrepareInvalidSqlThrows) {
  Database db = make_db_with_table();
  EXPECT_THROW((void)db.prepare("SELECT * FROM does_not_exist;"), DatabaseError);
}

TEST(DatabaseTest, ExecuteInvalidSqlThrows) {
  Database db = Database::in_memory();
  EXPECT_THROW(db.execute("THIS IS NOT SQL;"), DatabaseError);
}

TEST(DatabaseTest, NotNullConstraintSurfacesAsDatabaseError) {
  Database db = make_db_with_table();
  Statement insert = db.prepare("INSERT INTO t (name) VALUES (?);");
  insert.bind(1, nullptr);  // name is NOT NULL
  try {
    (void)insert.step();
    FAIL() << "expected DatabaseError for NOT NULL violation";
  } catch (const DatabaseError& e) {
    EXPECT_NE(e.code(), 0);
  }
}

TEST(DatabaseTest, MoveLeavesSourceSafeToDestroy) {
  Database db = make_db_with_table();
  Database moved = std::move(db);
  // `moved` still works after the move.
  moved.execute("INSERT INTO t (name) VALUES ('x');");
  EXPECT_GT(moved.last_insert_rowid(), 0);
}

}  // namespace
