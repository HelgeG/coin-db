#include "coins/db/sqlite_reference_link_repository.hpp"

#include <gtest/gtest.h>

#include <string_view>

#include "coins/db/database.hpp"
#include "coins/db/schema.hpp"
#include "coins/db/statement.hpp"
#include "coins/id.hpp"
#include "coins/reference_link.hpp"

namespace {

using coins::Id;
using coins::ReferenceLink;
using coins::db::Database;
using coins::db::SqliteReferenceLinkRepository;

// Inserts a minimal coin directly and returns its id (links need a valid FK).
Id insert_coin(Database& db) {
  coins::db::Statement stmt = db.prepare(
      "INSERT INTO coin (country, year_from, year_to, created_at, updated_at) "
      "VALUES ('Norway', 1963, 1963, '2026-01-01', '2026-01-01');");
  (void)stmt.step();
  return db.last_insert_rowid();
}

class SqliteReferenceLinkRepositoryTest : public ::testing::Test {
 protected:
  SqliteReferenceLinkRepositoryTest() : db_(Database::in_memory()) {
    coins::db::bootstrap_schema(db_);
    coin_id_ = insert_coin(db_);
  }

  ReferenceLink make_link() {
    ReferenceLink link;
    link.coin_id = coin_id_;
    link.label = "Numista";
    link.url = "https://numista.com/x";
    return link;
  }

  Database db_;
  Id coin_id_ = 0;
};

TEST_F(SqliteReferenceLinkRepositoryTest, AddAssignsIdAndListReturnsIt) {
  SqliteReferenceLinkRepository repo{db_};
  const auto added = repo.add(make_link());
  ASSERT_TRUE(added.has_value());
  EXPECT_GT(added->id, 0);

  const auto links = repo.list(coin_id_);
  ASSERT_EQ(links.size(), 1U);
  EXPECT_EQ(links[0].label, "Numista");
  EXPECT_EQ(links[0].url, "https://numista.com/x");
}

TEST_F(SqliteReferenceLinkRepositoryTest, AddRejectsInvalidLink) {
  SqliteReferenceLinkRepository repo{db_};
  ReferenceLink bad = make_link();
  bad.url = "not-a-url";
  EXPECT_FALSE(repo.add(bad).has_value());
  EXPECT_TRUE(repo.list(coin_id_).empty());
}

TEST_F(SqliteReferenceLinkRepositoryTest, RemoveDeletesLink) {
  SqliteReferenceLinkRepository repo{db_};
  const auto added = repo.add(make_link());
  ASSERT_TRUE(added.has_value());

  EXPECT_TRUE(repo.remove(added->id));
  EXPECT_TRUE(repo.list(coin_id_).empty());
  EXPECT_FALSE(repo.remove(added->id));  // already gone
}

TEST_F(SqliteReferenceLinkRepositoryTest, LinksCascadeWhenCoinDeleted) {
  SqliteReferenceLinkRepository repo{db_};
  ASSERT_TRUE(repo.add(make_link()).has_value());

  db_.execute("DELETE FROM coin;");
  EXPECT_TRUE(repo.list(coin_id_).empty());
}

}  // namespace
