#ifndef COINS_COIN_QUERY_HPP
#define COINS_COIN_QUERY_HPP

#include <optional>
#include <string>

namespace coins {

/// Field to sort search results by.
enum class SortField {
  DateAdded,  // created_at
  Year,       // year_from
  Country,    // country
  ValueEur    // latest EUR estimate (coins without an estimate sort last)
};

enum class SortDirection { Ascending, Descending };

/// Criteria for searching/filtering/sorting the collection. Every filter is
/// optional; an all-empty query returns the whole collection. Filters combine
/// with AND. Matching rules:
///   - `country`, `denomination`, `composition`: match the coin's lookup entry
///     by localized name (case-insensitive substring, any language) or by code.
///   - `grade_label`: case-insensitive exact match.
///   - `year_from`/`year_to`: range overlap against each coin's [year_from,
///     year_to] (either bound may be given alone for an open-ended range).
///   - `min_value_eur`/`max_value_eur`: bound the coin's latest EUR estimate
///     (coins without an estimate are excluded when a value bound is set).
///   - `text`: case-insensitive substring across the coin's localized lookup
///     names, notes, and reference-link labels.
///   - `lang`: active language for the Country sort (defaults to English).
struct CoinQuery {
  std::optional<std::string> country;
  std::optional<int> year_from;
  std::optional<int> year_to;
  std::optional<std::string> denomination;
  std::optional<std::string> grade_label;
  std::optional<std::string> composition;
  std::optional<double> min_value_eur;
  std::optional<double> max_value_eur;
  std::optional<std::string> text;
  std::string lang = "en";

  SortField sort_field = SortField::DateAdded;
  SortDirection sort_direction = SortDirection::Ascending;
};

}  // namespace coins

#endif  // COINS_COIN_QUERY_HPP
