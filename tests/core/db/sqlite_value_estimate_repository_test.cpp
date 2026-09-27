#include "coins/db/sqlite_value_estimate_repository.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <string>

#include "coins/db/database.hpp"
#include "coins/db/schema.hpp"
#include "coins/db/statement.hpp"
#include "coins/id.hpp"
#include "coins/value_estimate.hpp"

namespace {

using coins::Id;
using coins::ValueEstimate;
using coins::db::Database;
using coins::db::SqliteValueEstimateRepository;

Id insert_coin(Database& db) {
  // country_id references a seeded lookup entry (FK is enforced).
  coins::db::Statement stmt = db.prepare(
      "INSERT INTO coin (country_id, year_from, year_to, created_at, updated_at) "
      "SELECT id, 1963, 1963, '2026-01-01', '2026-01-01' FROM lookup_entry "
      "WHERE kind = 'country' LIMIT 1;");
  (void)stmt.step();
  return db.last_insert_rowid();
}

class SqliteValueEstimateRepositoryTest : public ::testing::Test {
 protected:
  SqliteValueEstimateRepositoryTest() : db_(Database::in_memory()) {
    coins::db::bootstrap_schema(db_);
    coin_id_ = insert_coin(db_);
  }

  ValueEstimate estimate(double amount, std::string date) {
    ValueEstimate est;
    est.coin_id = coin_id_;
    est.amount = amount;
    est.estimated_at = std::move(date);
    return est;
  }

  Database db_;
  Id coin_id_ = 0;
};

TEST_F(SqliteValueEstimateRepositoryTest, AddAppendsAndAssignsId) {
  SqliteValueEstimateRepository repo{db_};
  const auto added = repo.add(estimate(10.0, "2026-01-01"));
  ASSERT_TRUE(added.has_value());
  EXPECT_GT(added->id, 0);
}

TEST_F(SqliteValueEstimateRepositoryTest, AddRejectsInvalidEstimate) {
  SqliteValueEstimateRepository repo{db_};
  EXPECT_FALSE(repo.add(estimate(-5.0, "2026-01-01")).has_value());
  EXPECT_FALSE(repo.add(estimate(5.0, "nope")).has_value());
  EXPECT_TRUE(repo.list_for_coin(coin_id_).empty());
}

TEST_F(SqliteValueEstimateRepositoryTest, HistoryIsAppendOnlyAndOrdered) {
  SqliteValueEstimateRepository repo{db_};
  ASSERT_TRUE(repo.add(estimate(20.0, "2026-03-01")).has_value());
  ASSERT_TRUE(repo.add(estimate(10.0, "2026-01-01")).has_value());
  ASSERT_TRUE(repo.add(estimate(15.0, "2026-02-01")).has_value());

  const auto history = repo.list_for_coin(coin_id_);
  ASSERT_EQ(history.size(), 3U);  // nothing overwritten
  EXPECT_EQ(history[0].estimated_at, "2026-01-01");
  EXPECT_EQ(history[1].estimated_at, "2026-02-01");
  EXPECT_EQ(history[2].estimated_at, "2026-03-01");
}

TEST_F(SqliteValueEstimateRepositoryTest, LatestForCoinReturnsMostRecent) {
  SqliteValueEstimateRepository repo{db_};
  ASSERT_TRUE(repo.add(estimate(10.0, "2026-01-01")).has_value());
  ASSERT_TRUE(repo.add(estimate(30.0, "2026-03-01")).has_value());
  ASSERT_TRUE(repo.add(estimate(20.0, "2026-02-01")).has_value());

  const auto latest = repo.latest_for_coin(coin_id_);
  ASSERT_TRUE(latest.has_value());
  EXPECT_EQ(latest->estimated_at, "2026-03-01");
  EXPECT_DOUBLE_EQ(latest->amount, 30.0);
}

TEST_F(SqliteValueEstimateRepositoryTest, LatestForCoinBreaksTiesByIdInsertionOrder) {
  SqliteValueEstimateRepository repo{db_};
  ASSERT_TRUE(repo.add(estimate(10.0, "2026-05-01")).has_value());
  const auto second = repo.add(estimate(99.0, "2026-05-01"));  // same date, later id
  ASSERT_TRUE(second.has_value());

  const auto latest = repo.latest_for_coin(coin_id_);
  ASSERT_TRUE(latest.has_value());
  EXPECT_EQ(latest->id, second->id);
  EXPECT_DOUBLE_EQ(latest->amount, 99.0);
}

TEST_F(SqliteValueEstimateRepositoryTest, LatestForCoinWithNoEstimatesIsNullopt) {
  SqliteValueEstimateRepository repo{db_};
  EXPECT_FALSE(repo.latest_for_coin(coin_id_).has_value());
}

}  // namespace
