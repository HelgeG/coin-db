#ifndef COINS_SUMMARY_TYPE_HPP
#define COINS_SUMMARY_TYPE_HPP

#include <array>
#include <optional>
#include <string_view>

namespace coins {

/// The predefined kinds of collection summary the app can display. Each maps to
/// one aggregate query in `SummaryService`. Adding a new kind is additive: add
/// an enum value, its wire key below, and its query — callers stay unchanged.
enum class SummaryType {
  ByCountry,   ///< Coin count grouped by country (default).
  TotalValue,  ///< Headline EUR total only; no breakdown buckets.
  ByDecade,    ///< Coin count grouped by decade of `year_from`.
  ByGrade,     ///< Coin count grouped by grade label.
  ByMetal,     ///< Coin count grouped by composition / metal.
};

/// The default summary type used when none is specified.
inline constexpr SummaryType kDefaultSummaryType = SummaryType::ByCountry;

/// Stable wire/CLI key for a summary type (e.g. "by_country").
[[nodiscard]] constexpr std::string_view to_string(SummaryType type) noexcept {
  switch (type) {
    case SummaryType::ByCountry:
      return "by_country";
    case SummaryType::TotalValue:
      return "total_value";
    case SummaryType::ByDecade:
      return "by_decade";
    case SummaryType::ByGrade:
      return "by_grade";
    case SummaryType::ByMetal:
      return "by_metal";
  }
  return "by_country";
}

/// All summary types, in display order.
inline constexpr std::array<SummaryType, 5> kAllSummaryTypes = {
    SummaryType::ByCountry, SummaryType::TotalValue, SummaryType::ByDecade, SummaryType::ByGrade,
    SummaryType::ByMetal};

/// Parses a wire/CLI key into a `SummaryType`. Returns `std::nullopt` for an
/// unknown key so callers can decide how to handle it (e.g. fall back to the
/// default, or report an error).
[[nodiscard]] constexpr std::optional<SummaryType> summary_type_from_string(
    std::string_view key) noexcept {
  for (const SummaryType type : kAllSummaryTypes) {
    if (to_string(type) == key) {
      return type;
    }
  }
  return std::nullopt;
}

}  // namespace coins

#endif  // COINS_SUMMARY_TYPE_HPP
