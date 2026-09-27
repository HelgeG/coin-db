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
#include "coins/summary_type.hpp"
#include "coins/value_estimate.hpp"

namespace {

using coins::Coin;
using coins::CollectionSummary;
using coins::Id;
using coins::SummaryBucket;
using coins::SummaryService;
using coins::SummaryType;
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

  /// Adds a bare Norway/1963 coin (used by the value-total tests).
  Id add_coin() {
    Coin coin;
    coin.country = "Norway";
    coin.year_from = 1963;
    coin.year_to = 1963;
    const auto created = coins_.create(coin);
    EXPECT_TRUE(created.has_value());
    return created->id;
  }

  /// Adds a coin with the attributes the breakdown tests group on. Empty
  /// `grade`/`composition` are stored as NULL to exercise the null buckets.
  Id add_coin(std::string country, int year, std::string grade, std::string composition) {
    Coin coin;
    coin.country = std::move(country);
    coin.year_from = year;
    coin.year_to = year;
    if (!grade.empty()) coin.grade_label = std::move(grade);
    if (!composition.empty()) coin.composition = std::move(composition);
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
  EXPECT_TRUE(result.breakdown.buckets.empty());
}

TEST_F(SummaryServiceTest, DefaultTypeIsByCountry) {
  SummaryService summary{db_};
  EXPECT_EQ(summary.summarize().breakdown.type, SummaryType::ByCountry);
}

TEST_F(SummaryServiceTest, TotalUsesLatestEstimatePerCoin) {
  const Id a = add_coin();
  const Id b = add_coin();
  add_coin();  // coin with no estimate contributes 0

  add_estimate(a, 50.0, "2026-01-01");
  add_estimate(a, 100.0, "2026-02-01");  // latest for a
  add_estimate(b, 200.0, "2026-01-15");

  SummaryService summary{db_};
  const CollectionSummary result = summary.summarize();
  EXPECT_EQ(result.coin_count, 3);
  EXPECT_DOUBLE_EQ(result.total_estimate_eur, 300.0);  // 100 + 200 + 0
}

TEST_F(SummaryServiceTest, TotalValueTypeHasNoBuckets) {
  add_coin("Norway", 1963, "", "");
  add_coin("Sweden", 1970, "", "");

  SummaryService summary{db_};
  const CollectionSummary result = summary.summarize(SummaryType::TotalValue);
  EXPECT_EQ(result.coin_count, 2);
  EXPECT_EQ(result.breakdown.type, SummaryType::TotalValue);
  EXPECT_TRUE(result.breakdown.buckets.empty());
}

TEST_F(SummaryServiceTest, ByCountryGroupsAndSorts) {
  add_coin("Norway", 1963, "", "");
  add_coin("Norway", 1970, "", "");
  add_coin("Norway", 1980, "", "");
  add_coin("Sweden", 1963, "", "");
  add_coin("Denmark", 1963, "", "");
  add_coin("Sweden", 1970, "", "");

  SummaryService summary{db_};
  const CollectionSummary result = summary.summarize(SummaryType::ByCountry);

  const std::vector<SummaryBucket> expected{{"Norway", 3}, {"Sweden", 2}, {"Denmark", 1}};
  EXPECT_EQ(result.breakdown.buckets, expected);
}

TEST_F(SummaryServiceTest, ByDecadeGroupsByStartingYear) {
  add_coin("Norway", 1963, "", "");
  add_coin("Norway", 1968, "", "");  // same decade as 1963
  add_coin("Sweden", 1975, "", "");
  add_coin("Denmark", 2001, "", "");

  SummaryService summary{db_};
  const CollectionSummary result = summary.summarize(SummaryType::ByDecade);

  const std::vector<SummaryBucket> expected{{"1960s", 2}, {"1970s", 1}, {"2000s", 1}};
  EXPECT_EQ(result.breakdown.buckets, expected);
}

TEST_F(SummaryServiceTest, ByGradeGroupsAndBucketsUngraded) {
  add_coin("Norway", 1963, "MS", "");
  add_coin("Norway", 1970, "MS", "");
  add_coin("Sweden", 1963, "VF", "");
  add_coin("Denmark", 1963, "", "");  // no grade -> ungraded bucket

  SummaryService summary{db_};
  const CollectionSummary result = summary.summarize(SummaryType::ByGrade);

  const std::vector<SummaryBucket> expected{{"MS", 2}, {"VF", 1}, {"(ungraded)", 1}};
  EXPECT_EQ(result.breakdown.buckets, expected);
}

TEST_F(SummaryServiceTest, ByMetalGroupsAndBucketsUnknown) {
  add_coin("Norway", 1963, "", "Silver .900");
  add_coin("Norway", 1970, "", "Silver .900");
  add_coin("Sweden", 1963, "", "Bronze");
  add_coin("Denmark", 1963, "", "");  // no composition -> unknown bucket

  SummaryService summary{db_};
  const CollectionSummary result = summary.summarize(SummaryType::ByMetal);

  const std::vector<SummaryBucket> expected{{"Silver .900", 2}, {"Bronze", 1}, {"(unknown)", 1}};
  EXPECT_EQ(result.breakdown.buckets, expected);
}

}  // namespace
