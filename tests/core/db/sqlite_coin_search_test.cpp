#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "coins/clock.hpp"
#include "coins/coin.hpp"
#include "coins/coin_query.hpp"
#include "coins/db/database.hpp"
#include "coins/db/schema.hpp"
#include "coins/db/sqlite_coin_repository.hpp"
#include "coins/db/sqlite_lookup_repository.hpp"
#include "coins/db/sqlite_reference_link_repository.hpp"
#include "coins/db/sqlite_value_estimate_repository.hpp"
#include "coins/id.hpp"
#include "coins/lookup_kind.hpp"
#include "coins/lookup_service.hpp"
#include "coins/reference_link.hpp"
#include "coins/value_estimate.hpp"

namespace {

using coins::Coin;
using coins::CoinQuery;
using coins::Id;
using coins::LookupKind;
using coins::LookupService;
using coins::SortDirection;
using coins::SortField;
using coins::db::Database;
using coins::db::SqliteCoinRepository;
using coins::db::SqliteLookupRepository;
using coins::db::SqliteReferenceLinkRepository;
using coins::db::SqliteValueEstimateRepository;

class FixedClock final : public coins::IClock {
 public:
  [[nodiscard]] std::string now_iso8601() const override { return "2026-01-01T00:00:00Z"; }
};

std::vector<Id> ids_of(const std::vector<Coin>& coins) {
  std::vector<Id> ids;
  ids.reserve(coins.size());
  for (const Coin& coin : coins) {
    ids.push_back(coin.id);
  }
  return ids;
}

// A small fixed collection covering the filter/sort dimensions:
//   c1 Norway 1963  "50 Øre"    Bronze  grade 1+  notes "gift"    latest 100
//   c2 Norway 1990  "10 Kroner" Silver  grade 01                  latest 50
//   c3 Sweden 1950-1955 "1 Krona" Silver  notes "rare"            latest 300
//   c4 USA    1889  "1 Dollar"  Silver  notes "Morgan"  (no estimate; link "Numista Morgan")
// The encoded fields (country, denomination, composition) are stored as lookup
// entries; search filters still match by their localized name (or code).
class SqliteCoinSearchTest : public ::testing::Test {
 protected:
  SqliteCoinSearchTest()
      : db_(Database::in_memory()),
        lookups_(db_),
        lookup_svc_(lookups_),
        coins_(db_, clock_),
        estimates_(db_),
        links_(db_) {
    coins::db::bootstrap_schema(db_);
    c1_ = add_coin("Norway", 1963, 1963, "50 Øre", "Bronze", "1+", "gift from grandpa");
    c2_ = add_coin("Norway", 1990, 1990, "10 Kroner", "Silver", "01", std::nullopt);
    c3_ = add_coin("Sweden", 1950, 1955, "1 Krona", "Silver", std::nullopt, "rare find");
    c4_ = add_coin("USA", 1889, 1889, "1 Dollar", "Silver", std::nullopt, "Morgan");

    add_estimate(c1_, 100.0);
    add_estimate(c2_, 50.0);
    add_estimate(c3_, 300.0);
    // c4 has no estimate.

    coins::ReferenceLink link;
    link.coin_id = c4_;
    link.label = "Numista Morgan";
    link.url = "https://numista.com/x";
    EXPECT_TRUE(links_.add(link).has_value());
  }

  Id lookup_id(LookupKind kind, std::string_view name) {
    return lookup_svc_.resolve_or_create(kind, "en", name).id;
  }

  Id add_coin(std::string_view country, int year_from, int year_to,
              std::optional<std::string> denomination, std::optional<std::string> composition,
              std::optional<std::string> grade_label, std::optional<std::string> notes) {
    Coin coin;
    coin.country_id = lookup_id(LookupKind::Country, country);
    coin.year_from = year_from;
    coin.year_to = year_to;
    if (denomination.has_value()) {
      coin.denomination_id = lookup_id(LookupKind::Denomination, *denomination);
    }
    if (composition.has_value()) {
      coin.composition_id = lookup_id(LookupKind::Composition, *composition);
    }
    if (grade_label.has_value()) {
      coin.grade_scale = "Norwegian";
      coin.grade_label = std::move(grade_label);
    }
    coin.notes = std::move(notes);
    const auto created = coins_.create(coin);
    EXPECT_TRUE(created.has_value());
    return created->id;
  }

  void add_estimate(Id coin_id, double amount) {
    coins::ValueEstimate est;
    est.coin_id = coin_id;
    est.amount = amount;
    est.estimated_at = "2026-01-01";
    EXPECT_TRUE(estimates_.add(est).has_value());
  }

  Database db_;
  FixedClock clock_;
  SqliteLookupRepository lookups_;
  LookupService lookup_svc_;
  SqliteCoinRepository coins_;
  SqliteValueEstimateRepository estimates_;
  SqliteReferenceLinkRepository links_;
  Id c1_ = 0, c2_ = 0, c3_ = 0, c4_ = 0;
};

TEST_F(SqliteCoinSearchTest, EmptyQueryReturnsAll) {
  EXPECT_EQ(coins_.search(CoinQuery{}).size(), 4U);
}

TEST_F(SqliteCoinSearchTest, FilterByCountryIsCaseInsensitive) {
  CoinQuery q;
  q.country = "norway";
  EXPECT_EQ(ids_of(coins_.search(q)), (std::vector<Id>{c1_, c2_}));
}

TEST_F(SqliteCoinSearchTest, FilterByYearRangeOverlap) {
  CoinQuery q;
  q.year_from = 1952;
  q.year_to = 1952;  // a single year that falls inside c3's 1950-1955 range
  EXPECT_EQ(ids_of(coins_.search(q)), (std::vector<Id>{c3_}));
}

TEST_F(SqliteCoinSearchTest, FilterByDenominationSubstring) {
  CoinQuery q;
  q.denomination = "dollar";
  EXPECT_EQ(ids_of(coins_.search(q)), (std::vector<Id>{c4_}));
}

TEST_F(SqliteCoinSearchTest, FilterByGradeLabel) {
  CoinQuery q;
  q.grade_label = "1+";
  EXPECT_EQ(ids_of(coins_.search(q)), (std::vector<Id>{c1_}));
}

TEST_F(SqliteCoinSearchTest, FilterByComposition) {
  CoinQuery q;
  q.composition = "silver";
  EXPECT_EQ(ids_of(coins_.search(q)), (std::vector<Id>{c2_, c3_, c4_}));
}

TEST_F(SqliteCoinSearchTest, FilterByValueRangeUsesLatestEstimate) {
  CoinQuery q;
  q.min_value_eur = 80.0;  // c1=100, c3=300 qualify; c2=50 and c4=none excluded
  EXPECT_EQ(ids_of(coins_.search(q)), (std::vector<Id>{c1_, c3_}));

  q.max_value_eur = 120.0;  // now only c1=100
  EXPECT_EQ(ids_of(coins_.search(q)), (std::vector<Id>{c1_}));
}

TEST_F(SqliteCoinSearchTest, SortByYearAscending) {
  CoinQuery q;
  q.sort_field = SortField::Year;
  q.sort_direction = SortDirection::Ascending;
  EXPECT_EQ(ids_of(coins_.search(q)), (std::vector<Id>{c4_, c3_, c1_, c2_}));
}

TEST_F(SqliteCoinSearchTest, SortByValueDescendingPutsCoinsWithoutEstimateLast) {
  CoinQuery q;
  q.sort_field = SortField::ValueEur;
  q.sort_direction = SortDirection::Descending;
  EXPECT_EQ(ids_of(coins_.search(q)), (std::vector<Id>{c3_, c1_, c2_, c4_}));
}

TEST_F(SqliteCoinSearchTest, FreeTextMatchesNotesAndCountryAndDenomination) {
  CoinQuery q;
  q.text = "swed";  // matches c3's country
  EXPECT_EQ(ids_of(coins_.search(q)), (std::vector<Id>{c3_}));
}

TEST_F(SqliteCoinSearchTest, FreeTextMatchesReferenceLabel) {
  CoinQuery q;
  q.text = "numista";  // only via c4's reference-link label
  EXPECT_EQ(ids_of(coins_.search(q)), (std::vector<Id>{c4_}));
}

}  // namespace
