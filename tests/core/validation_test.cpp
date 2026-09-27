#include "coins/validation.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string_view>

#include "coins/coin.hpp"
#include "coins/reference_link.hpp"
#include "coins/value_estimate.hpp"

namespace {

using coins::Coin;
using coins::ReferenceLink;
using coins::ValidationErrors;
using coins::ValueEstimate;

// A minimally valid coin: only the required fields set. `country_id` references
// a lookup entry, so any positive id is enough for shape-level validation.
Coin make_valid_coin() {
  Coin coin;
  coin.country_id = 1;
  coin.year_from = 1963;
  coin.year_to = 1963;
  return coin;
}

bool has_error_for(const ValidationErrors& errors, std::string_view field) {
  return std::any_of(errors.begin(), errors.end(),
                     [field](const coins::ValidationError& e) { return e.field == field; });
}

TEST(ValidationTest, MinimalValidCoinPasses) {
  EXPECT_TRUE(coins::validate_coin(make_valid_coin()).has_value());
}

TEST(ValidationTest, MissingCountryFails) {
  Coin coin = make_valid_coin();
  coin.country_id = 0;  // unset
  const auto result = coins::validate_coin(coin);
  ASSERT_FALSE(result.has_value());
  EXPECT_TRUE(has_error_for(result.error(), "country"));
}

TEST(ValidationTest, NonPositiveCountryIdFails) {
  Coin coin = make_valid_coin();
  coin.country_id = -1;
  const auto result = coins::validate_coin(coin);
  ASSERT_FALSE(result.has_value());
  EXPECT_TRUE(has_error_for(result.error(), "country"));
}

TEST(ValidationTest, MissingYearFails) {
  Coin coin = make_valid_coin();
  coin.year_from = 0;
  coin.year_to = 0;
  const auto result = coins::validate_coin(coin);
  ASSERT_FALSE(result.has_value());
  EXPECT_TRUE(has_error_for(result.error(), "year_from"));
}

TEST(ValidationTest, InvertedYearRangeFails) {
  Coin coin = make_valid_coin();
  coin.year_from = 1990;
  coin.year_to = 1980;
  const auto result = coins::validate_coin(coin);
  ASSERT_FALSE(result.has_value());
  EXPECT_TRUE(has_error_for(result.error(), "year_to"));
}

TEST(ValidationTest, YearRangePasses) {
  Coin coin = make_valid_coin();
  coin.year_from = 1980;
  coin.year_to = 1990;
  EXPECT_TRUE(coins::validate_coin(coin).has_value());
}

TEST(ValidationTest, NegativeMoneyOrMeasurementFails) {
  Coin coin = make_valid_coin();
  coin.face_value = -1.0;
  coin.weight_g = -2.0;
  const auto result = coins::validate_coin(coin);
  ASSERT_FALSE(result.has_value());
  EXPECT_TRUE(has_error_for(result.error(), "face_value"));
  EXPECT_TRUE(has_error_for(result.error(), "weight_g"));
}

TEST(ValidationTest, SheldonGradeRangeIsEnforced) {
  Coin coin = make_valid_coin();
  coin.grade_scale = "Sheldon";
  coin.grade_numeric = 65;
  EXPECT_TRUE(coins::validate_coin(coin).has_value());

  for (const int bad : {0, 71, -3}) {
    coin.grade_numeric = bad;
    const auto result = coins::validate_coin(coin);
    ASSERT_FALSE(result.has_value()) << "expected rejection for Sheldon " << bad;
    EXPECT_TRUE(has_error_for(result.error(), "grade_numeric"));
  }
}

TEST(ValidationTest, NorwegianGradeAcceptsValidLabels) {
  for (const auto* label : {"0", "0/01", "01", "1+", "1", "1-", "2", "3"}) {
    Coin coin = make_valid_coin();
    coin.grade_scale = "Norwegian";
    coin.grade_label = label;
    EXPECT_TRUE(coins::validate_coin(coin).has_value()) << "expected '" << label << "' to be valid";
  }
}

TEST(ValidationTest, NorwegianGradeRejectsUnknownLabel) {
  Coin coin = make_valid_coin();
  coin.grade_scale = "Norwegian";
  coin.grade_label = "5";
  const auto result = coins::validate_coin(coin);
  ASSERT_FALSE(result.has_value());
  EXPECT_TRUE(has_error_for(result.error(), "grade_label"));
}

TEST(ValidationTest, NorwegianGradeRequiresLabel) {
  Coin coin = make_valid_coin();
  coin.grade_scale = "Norwegian";  // no label
  const auto result = coins::validate_coin(coin);
  ASSERT_FALSE(result.has_value());
  EXPECT_TRUE(has_error_for(result.error(), "grade_label"));
}

TEST(ValidationTest, UnknownScaleIsAcceptedAsIs) {
  Coin coin = make_valid_coin();
  coin.grade_scale = "British";  // adjectival system the core does not constrain
  coin.grade_label = "VF";
  EXPECT_TRUE(coins::validate_coin(coin).has_value());
}

TEST(ValidationTest, CollectsAllErrors) {
  Coin coin;  // unset country_id, zero years
  const auto result = coins::validate_coin(coin);
  ASSERT_FALSE(result.has_value());
  EXPECT_GE(result.error().size(), 2U);
  EXPECT_TRUE(has_error_for(result.error(), "country"));
  EXPECT_TRUE(has_error_for(result.error(), "year_from"));
}

TEST(ValidationTest, AcceptsValidUrls) {
  EXPECT_TRUE(coins::is_valid_url("https://numista.com/catalogue/pieces123.html"));
  EXPECT_TRUE(coins::is_valid_url("http://example.org"));
  EXPECT_TRUE(coins::is_valid_url("https://a.b/c?d=e#f"));
}

TEST(ValidationTest, RejectsInvalidUrls) {
  EXPECT_FALSE(coins::is_valid_url("ftp://example.org"));   // wrong scheme
  EXPECT_FALSE(coins::is_valid_url("example.org"));         // no scheme
  EXPECT_FALSE(coins::is_valid_url("https://"));            // empty host
  EXPECT_FALSE(coins::is_valid_url("http:// spaced.com"));  // space in host
  EXPECT_FALSE(coins::is_valid_url(""));
}

TEST(ValidationTest, ValidReferenceLinkPasses) {
  ReferenceLink link;
  link.label = "Numista";
  link.url = "https://numista.com/x";
  EXPECT_TRUE(coins::validate_reference_link(link).has_value());
}

TEST(ValidationTest, ReferenceLinkRequiresLabelAndValidUrl) {
  ReferenceLink link;
  link.label = "";
  link.url = "not-a-url";
  const auto result = coins::validate_reference_link(link);
  ASSERT_FALSE(result.has_value());
  EXPECT_TRUE(has_error_for(result.error(), "label"));
  EXPECT_TRUE(has_error_for(result.error(), "url"));
}

TEST(ValidationTest, ValidValueEstimatePasses) {
  ValueEstimate estimate;
  estimate.amount = 12.50;
  estimate.estimated_at = "2026-01-15";
  EXPECT_TRUE(coins::validate_value_estimate(estimate).has_value());
}

TEST(ValidationTest, ValueEstimateRejectsNegativeAmount) {
  ValueEstimate estimate;
  estimate.amount = -1.0;
  estimate.estimated_at = "2026-01-15";
  const auto result = coins::validate_value_estimate(estimate);
  ASSERT_FALSE(result.has_value());
  EXPECT_TRUE(has_error_for(result.error(), "amount"));
}

TEST(ValidationTest, ValueEstimateRejectsBadDate) {
  for (const auto* bad : {"", "2026", "2026/01/15", "15-01-2026", "2026-1-5"}) {
    ValueEstimate estimate;
    estimate.amount = 5.0;
    estimate.estimated_at = bad;
    const auto result = coins::validate_value_estimate(estimate);
    ASSERT_FALSE(result.has_value()) << "expected rejection for '" << bad << "'";
    EXPECT_TRUE(has_error_for(result.error(), "estimated_at"));
  }
}

}  // namespace
