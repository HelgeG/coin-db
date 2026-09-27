#include "coins/collection_io.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <string_view>

#include "coins/clock.hpp"
#include "coins/coin.hpp"
#include "coins/db/database.hpp"
#include "coins/db/schema.hpp"
#include "coins/db/sqlite_coin_repository.hpp"
#include "coins/db/sqlite_lookup_repository.hpp"
#include "coins/db/sqlite_reference_link_repository.hpp"
#include "coins/db/sqlite_value_estimate_repository.hpp"
#include "coins/db/statement.hpp"
#include "coins/id.hpp"
#include "coins/lookup_kind.hpp"
#include "coins/lookup_service.hpp"
#include "coins/reference_link.hpp"
#include "coins/value_estimate.hpp"

namespace {

using coins::Coin;
using coins::Id;
using coins::LookupKind;
using coins::LookupService;
using coins::db::Database;
using coins::db::SqliteCoinRepository;
using coins::db::SqliteLookupRepository;
using coins::db::SqliteReferenceLinkRepository;
using coins::db::SqliteValueEstimateRepository;

class FixedClock final : public coins::IClock {
 public:
  [[nodiscard]] std::string now_iso8601() const override { return "2026-01-01T00:00:00Z"; }
};

std::size_t line_count(std::string_view text) {
  return static_cast<std::size_t>(std::count(text.begin(), text.end(), '\n'));
}

void insert_image_row(Database& db, Id coin_id, std::string_view kind,
                      std::string_view stored_path) {
  coins::db::Statement stmt =
      db.prepare("INSERT INTO image (coin_id, kind, stored_path) VALUES (?, ?, ?);");
  stmt.bind(1, coin_id);
  stmt.bind(2, kind);
  stmt.bind(3, stored_path);
  (void)stmt.step();
}

class CollectionIoTest : public ::testing::Test {
 protected:
  CollectionIoTest() : db_(Database::in_memory()) {
    coins::db::bootstrap_schema(db_);
    SqliteLookupRepository lookups{db_};
    LookupService svc{lookups};
    SqliteCoinRepository coins{db_, clock_};
    SqliteValueEstimateRepository estimates{db_};
    SqliteReferenceLinkRepository links{db_};

    auto id_of = [&](LookupKind kind, std::string_view name) {
      return svc.resolve_or_create(kind, "en", name).id;
    };

    // Only seeded lookup entries (countries + currencies) are referenced so the
    // ids are reproduced identically in a freshly-seeded DB. (export_json does
    // not yet carry a lookups section, so created entries would not round-trip.)
    // Reference them by ISO code, which resolve_or_create matches directly.
    Coin a;
    a.country_id = id_of(LookupKind::Country, "NO");
    a.year_from = 1963;
    a.year_to = 1963;
    a.currency_id = id_of(LookupKind::Currency, "NOK");
    a.face_value = 0.5;
    a.grade_scale = "Norwegian";
    a.grade_label = "1+";
    a.notes = "gift, with comma";
    const auto created_a = coins.create(a);
    EXPECT_TRUE(created_a.has_value());
    const Id a_id = created_a->id;

    coins::ValueEstimate e1;
    e1.coin_id = a_id;
    e1.amount_eur = 50.0;
    e1.estimated_at = "2026-01-01";
    EXPECT_TRUE(estimates.add(e1).has_value());
    coins::ValueEstimate e2;
    e2.coin_id = a_id;
    e2.amount_eur = 100.0;
    e2.estimated_at = "2026-02-01";
    EXPECT_TRUE(estimates.add(e2).has_value());

    coins::ReferenceLink link;
    link.coin_id = a_id;
    link.label = "Numista";
    link.url = "https://numista.com/x";
    EXPECT_TRUE(links.add(link).has_value());

    insert_image_row(db_, a_id, "obverse", std::to_string(a_id) + "/front.png");

    Coin b;
    b.country_id = id_of(LookupKind::Country, "US");
    b.year_from = 1889;
    b.year_to = 1889;
    b.currency_id = id_of(LookupKind::Currency, "USD");
    b.notes = "has a \"quote\" inside";
    EXPECT_TRUE(coins.create(b).has_value());
  }

  Database db_;
  FixedClock clock_;
};

TEST_F(CollectionIoTest, JsonRoundTripReproducesCollection) {
  const std::string exported = coins::export_json(db_);

  Database fresh = Database::in_memory();
  coins::db::bootstrap_schema(fresh);
  const auto stats = coins::import_json(fresh, exported);
  ASSERT_TRUE(stats.has_value());
  EXPECT_EQ(stats->coins, 2);
  EXPECT_EQ(stats->value_estimates, 2);
  EXPECT_EQ(stats->reference_links, 1);
  EXPECT_EQ(stats->images, 1);

  // Deterministic export ordered by id, with ids preserved, so a faithful
  // restore re-exports byte-for-byte identically.
  EXPECT_EQ(coins::export_json(fresh), exported);
}

TEST_F(CollectionIoTest, ImportRejectsInvalidGraphAndWritesNothing) {
  // country_id 0 fails validation (country is required).
  const std::string invalid = R"({"coins":[{"country_id":0,"year_from":0,"year_to":0}]})";

  Database fresh = Database::in_memory();
  coins::db::bootstrap_schema(fresh);
  const auto stats = coins::import_json(fresh, invalid);
  ASSERT_FALSE(stats.has_value());
  EXPECT_FALSE(stats.error().empty());

  coins::db::Statement count = fresh.prepare("SELECT COUNT(*) FROM coin;");
  ASSERT_TRUE(count.step());
  EXPECT_EQ(count.column_int64(0), 0);  // nothing written
}

TEST_F(CollectionIoTest, ImportRejectsMalformedJson) {
  Database fresh = Database::in_memory();
  coins::db::bootstrap_schema(fresh);
  const auto stats = coins::import_json(fresh, "{ this is not json");
  ASSERT_FALSE(stats.has_value());
  EXPECT_EQ(stats.error().front().field, "json");
}

TEST_F(CollectionIoTest, CsvExportHasHeaderAndOneRowPerCoin) {
  const std::string csv = coins::export_csv(db_);
  EXPECT_TRUE(csv.starts_with("id,country,denomination,"));
  EXPECT_EQ(line_count(csv), 3U);  // header + 2 coins

  // The comma-containing note is quoted so it stays in one field.
  EXPECT_NE(csv.find("\"gift, with comma\""), std::string::npos);
  // Latest estimate for coin A is the most recent (100.0).
  EXPECT_NE(csv.find("100"), std::string::npos);
  // Encoded fields render as their localized display name (not codes).
  EXPECT_NE(csv.find("Norway"), std::string::npos);
}

}  // namespace
