#include "coins/db/sqlite_coin_repository.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "coins/clock.hpp"
#include "coins/coin.hpp"
#include "coins/db/database.hpp"
#include "coins/db/schema.hpp"
#include "coins/db/sqlite_lookup_repository.hpp"
#include "coins/db/statement.hpp"
#include "coins/id.hpp"
#include "coins/lookup_kind.hpp"
#include "coins/lookup_service.hpp"
#include "support/rc_gtest.hpp"

namespace {

using coins::Coin;
using coins::Id;
using coins::LookupKind;
using coins::LookupService;
using coins::db::Database;
using coins::db::SqliteCoinRepository;
using coins::db::SqliteLookupRepository;

// Deterministic clock whose value can be advanced between operations.
class TestClock final : public coins::IClock {
 public:
  explicit TestClock(std::string value) : value_(std::move(value)) {}
  [[nodiscard]] std::string now_iso8601() const override { return value_; }
  void set(std::string value) { value_ = std::move(value); }

 private:
  std::string value_;
};

// Fixture: fresh in-memory DB with schema, a repository, and a fixed clock.
class SqliteCoinRepositoryTest : public ::testing::Test {
 protected:
  SqliteCoinRepositoryTest()
      : db_(Database::in_memory()),
        clock_("2026-01-01T00:00:00Z"),
        lookups_(db_),
        lookup_svc_(lookups_) {
    coins::db::bootstrap_schema(db_);
  }

  // Resolves a lookup entry by name (creating one if needed) and returns its id.
  Id lookup_id(LookupKind kind, std::string_view name) {
    return lookup_svc_.resolve_or_create(kind, "en", name).id;
  }

  Coin make_valid_coin() {
    Coin coin;
    coin.country_id = lookup_id(LookupKind::Country, "Norway");
    coin.year_from = 1963;
    coin.year_to = 1963;
    return coin;
  }

  SqliteCoinRepository repo() { return SqliteCoinRepository{db_, clock_}; }

  Database db_;
  TestClock clock_;
  SqliteLookupRepository lookups_;
  LookupService lookup_svc_;
};

TEST_F(SqliteCoinRepositoryTest, CreateAssignsIdAndTimestamps) {
  SqliteCoinRepository repository = repo();
  const auto created = repository.create(make_valid_coin());
  ASSERT_TRUE(created.has_value());
  EXPECT_GT(created->id, 0);
  EXPECT_EQ(created->created_at, "2026-01-01T00:00:00Z");
  EXPECT_EQ(created->updated_at, "2026-01-01T00:00:00Z");
}

TEST_F(SqliteCoinRepositoryTest, GetReturnsFullyPopulatedCoin) {
  Coin coin = make_valid_coin();
  coin.denomination_id = lookup_id(LookupKind::Denomination, "50 Øre");
  coin.face_value = 0.5;
  coin.currency_id = lookup_id(LookupKind::Currency, "NOK");
  coin.mint_id = lookup_id(LookupKind::Mint, "Kongsberg");
  coin.mint_mark = "KM";
  coin.composition_id = lookup_id(LookupKind::Composition, "Bronze");
  coin.weight_g = 4.65;
  coin.diameter_mm = 21.0;
  coin.grade_scale = "Norwegian";
  coin.grade_label = "1+";
  coin.acquired_date = "2025-12-24";
  coin.acquired_price = 3.25;
  coin.acquired_source = "auction";
  coin.notes = "gift";

  SqliteCoinRepository repository = repo();
  const auto created = repository.create(coin);
  ASSERT_TRUE(created.has_value());

  const auto fetched = repository.get(created->id);
  ASSERT_TRUE(fetched.has_value());
  EXPECT_EQ(*fetched, *created);
}

TEST_F(SqliteCoinRepositoryTest, GetMissingReturnsNullopt) {
  SqliteCoinRepository repository = repo();
  EXPECT_FALSE(repository.get(9999).has_value());
}

TEST_F(SqliteCoinRepositoryTest, ListReturnsAllInIdOrder) {
  SqliteCoinRepository repository = repo();
  const Id norway = lookup_id(LookupKind::Country, "Norway");
  const Id sweden = lookup_id(LookupKind::Country, "Sweden");
  Coin a = make_valid_coin();
  a.country_id = norway;
  Coin b = make_valid_coin();
  b.country_id = sweden;
  ASSERT_TRUE(repository.create(a).has_value());
  ASSERT_TRUE(repository.create(b).has_value());

  const auto all = repository.list();
  ASSERT_EQ(all.size(), 2U);
  EXPECT_EQ(all[0].country_id, norway);
  EXPECT_EQ(all[1].country_id, sweden);
  EXPECT_LT(all[0].id, all[1].id);
}

TEST_F(SqliteCoinRepositoryTest, UpdateChangesFieldsRefreshesUpdatedAtPreservesCreatedAt) {
  SqliteCoinRepository repository = repo();
  const auto created = repository.create(make_valid_coin());
  ASSERT_TRUE(created.has_value());

  clock_.set("2026-06-15T12:00:00Z");
  Coin edited = *created;
  edited.notes = "regraded";
  edited.grade_scale = "Norwegian";
  edited.grade_label = "01";

  const auto updated = repository.update(edited);
  ASSERT_TRUE(updated.has_value());
  EXPECT_TRUE(*updated);

  const auto fetched = repository.get(created->id);
  ASSERT_TRUE(fetched.has_value());
  EXPECT_EQ(fetched->notes, std::optional<std::string>{"regraded"});
  EXPECT_EQ(fetched->grade_label, std::optional<std::string>{"01"});
  EXPECT_EQ(fetched->created_at, "2026-01-01T00:00:00Z");  // preserved
  EXPECT_EQ(fetched->updated_at, "2026-06-15T12:00:00Z");  // refreshed
}

TEST_F(SqliteCoinRepositoryTest, UpdateNonExistentReturnsFalse) {
  SqliteCoinRepository repository = repo();
  Coin ghost = make_valid_coin();
  ghost.id = 4242;
  const auto updated = repository.update(ghost);
  ASSERT_TRUE(updated.has_value());
  EXPECT_FALSE(*updated);
}

TEST_F(SqliteCoinRepositoryTest, CreateInvalidReturnsErrorsAndPersistsNothing) {
  SqliteCoinRepository repository = repo();
  Coin invalid = make_valid_coin();
  invalid.country_id = 0;  // required
  const auto created = repository.create(invalid);
  ASSERT_FALSE(created.has_value());
  EXPECT_FALSE(created.error().empty());
  EXPECT_TRUE(repository.list().empty());
}

TEST_F(SqliteCoinRepositoryTest, UpdateInvalidReturnsErrors) {
  SqliteCoinRepository repository = repo();
  const auto created = repository.create(make_valid_coin());
  ASSERT_TRUE(created.has_value());

  Coin invalid = *created;
  invalid.year_from = 2000;
  invalid.year_to = 1990;  // inverted
  const auto updated = repository.update(invalid);
  EXPECT_FALSE(updated.has_value());
}

TEST_F(SqliteCoinRepositoryTest, RemoveDeletesExistingReturnsTrue) {
  SqliteCoinRepository repository = repo();
  const auto created = repository.create(make_valid_coin());
  ASSERT_TRUE(created.has_value());

  EXPECT_TRUE(repository.remove(created->id));
  EXPECT_FALSE(repository.get(created->id).has_value());
}

TEST_F(SqliteCoinRepositoryTest, RemoveMissingReturnsFalse) {
  SqliteCoinRepository repository = repo();
  EXPECT_FALSE(repository.remove(1234));
}

TEST_F(SqliteCoinRepositoryTest, RemoveCascadesToChildren) {
  SqliteCoinRepository repository = repo();
  const auto created = repository.create(make_valid_coin());
  ASSERT_TRUE(created.has_value());

  coins::db::Statement insert =
      db_.prepare("INSERT INTO value_estimate (coin_id, amount, estimated_at) VALUES (?, ?, ?);");
  insert.bind(1, created->id);
  insert.bind(2, 100.0);
  insert.bind(3, std::string_view{"2026-01-01"});
  ASSERT_FALSE(insert.step());

  ASSERT_TRUE(repository.remove(created->id));

  coins::db::Statement count = db_.prepare("SELECT count(*) FROM value_estimate;");
  ASSERT_TRUE(count.step());
  EXPECT_EQ(count.column_int64(0), 0);
}

// Property: for any valid coin, create-then-get returns an identical coin. The
// classic round-trip property, now over the real SQLite repository.
RC_GTEST_PROP(SqliteCoinRepositoryProperty, CreateThenGetRoundTrips, ()) {
  Database db = Database::in_memory();
  coins::db::bootstrap_schema(db);
  SqliteLookupRepository lookups{db};
  LookupService svc{lookups};

  auto id_of = [&](LookupKind kind, std::string_view name) {
    return svc.resolve_or_create(kind, "en", name).id;
  };

  Coin coin;
  coin.country_id =
      id_of(LookupKind::Country, *rc::gen::element(std::string("Norway"), std::string("Sweden"),
                                                   std::string("USA"), std::string("Germany")));
  coin.year_from = *rc::gen::inRange(1, 2027);
  coin.year_to = coin.year_from + *rc::gen::inRange(0, 60);

  if (*rc::gen::arbitrary<bool>()) {
    coin.denomination_id = id_of(
        LookupKind::Denomination,
        *rc::gen::element(std::string("50 Øre"), std::string("1 Krone"), std::string("1 Dollar")));
  }
  if (*rc::gen::arbitrary<bool>()) {
    // Clean, finite doubles round-trip bit-for-bit through SQLite REAL.
    coin.face_value = static_cast<double>(*rc::gen::inRange(0, 100000)) / 100.0;
    coin.currency_id =
        id_of(LookupKind::Currency,
              *rc::gen::element(std::string("NOK"), std::string("USD"), std::string("EUR")));
  }
  if (*rc::gen::arbitrary<bool>()) {
    coin.weight_g = static_cast<double>(*rc::gen::inRange(0, 50000)) / 1000.0;
  }
  if (*rc::gen::arbitrary<bool>()) {
    coin.notes = *rc::gen::element(std::string("gift"), std::string("inherited"), std::string(""));
  }

  TestClock clock{"2026-02-02T10:00:00Z"};
  SqliteCoinRepository repository{db, clock};

  const auto created = repository.create(coin);
  RC_ASSERT(created.has_value());
  const auto fetched = repository.get(created->id);
  RC_ASSERT(fetched.has_value());
  RC_ASSERT(*fetched == *created);
}

}  // namespace
