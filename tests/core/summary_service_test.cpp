#include "coins/summary_service.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

#include "coins/clock.hpp"
#include "coins/coin.hpp"
#include "coins/collection_summary.hpp"
#include "coins/db/database.hpp"
#include "coins/db/schema.hpp"
#include "coins/db/sqlite_coin_repository.hpp"
#include "coins/db/sqlite_value_estimate_repository.hpp"
#include "coins/id.hpp"
#include "coins/value_estimate.hpp"

namespace {

using coins::Coin;
using coins::CollectionSummary;
using coins::FaceValueTotal;
using coins::Id;
using coins::SummaryService;
using coins::ValueEstimate;
using coins::db::Database;
using coins::db::SqliteCoinRepository;
using coins::db::SqliteValueEstimateRepository;

class FixedClock final : public coins::IClock {
 public:
  [[nodiscard]] std::string now_iso8601() const override { return "2026-01-01T00:00:00Z"; }
};

class SummaryServiceTest : public ::testing::Test {
 protected:
  SummaryServiceTest() : db_(Database::in_memory()), coins_(db_, clock_), estimates_(db_) {
    coins::db::bootstrap_schema(db_);
  }

  Id add_coin(std::optional<std::string> currency, std::optional<double> face) {
    Coin coin;
    coin.country = "Norway";
    coin.year_from = 1963;
    coin.year_to = 1963;
    coin.coin_currency = std::move(currency);
    coin.face_value = face;
    const auto created = coins_.create(coin);
    EXPECT_TRUE(created.has_value());
    return created->id;
  }

  void add_estimate(Id coin_id, double amount, std::string date) {
    ValueEstimate est;
    est.coin_id = coin_id;
    est.amount_eur = amount;
    est.estimated_at = std::move(date);
    EXPECT_TRUE(estimates_.add(est).has_value());
  }

  Database db_;
  FixedClock clock_;
  SqliteCoinRepository coins_;
  SqliteValueEstimateRepository estimates_;
};

TEST_F(SummaryServiceTest, EmptyCollectionIsAllZero) {
  SummaryService summary{db_};
  const CollectionSummary result = summary.summarize();
  EXPECT_EQ(result.coin_count, 0);
  EXPECT_DOUBLE_EQ(result.total_estimate_eur, 0.0);
  EXPECT_TRUE(result.face_value_by_currency.empty());
}

TEST_F(SummaryServiceTest, TotalUsesLatestEstimatePerCoin) {
  const Id a = add_coin(std::nullopt, std::nullopt);
  const Id b = add_coin(std::nullopt, std::nullopt);
  add_coin(std::nullopt, std::nullopt);  // coin with no estimate contributes 0

  add_estimate(a, 50.0, "2026-01-01");
  add_estimate(a, 100.0, "2026-02-01");  // latest for a
  add_estimate(b, 200.0, "2026-01-15");

  SummaryService summary{db_};
  const CollectionSummary result = summary.summarize();
  EXPECT_EQ(result.coin_count, 3);
  EXPECT_DOUBLE_EQ(result.total_estimate_eur, 300.0);  // 100 + 200 + 0
}

TEST_F(SummaryServiceTest, FaceValueGroupedByCurrency) {
  add_coin("NOK", 0.5);
  add_coin("NOK", 1.0);
  add_coin("USD", 0.25);
  add_coin(std::nullopt, 5.0);    // no currency -> excluded
  add_coin("SEK", std::nullopt);  // no face value -> excluded

  SummaryService summary{db_};
  const CollectionSummary result = summary.summarize();

  const std::vector<FaceValueTotal> expected{{"NOK", 1.5}, {"USD", 0.25}};
  EXPECT_EQ(result.face_value_by_currency, expected);
}

}  // namespace
