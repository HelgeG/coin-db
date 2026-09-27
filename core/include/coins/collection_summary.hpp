#ifndef COINS_COLLECTION_SUMMARY_HPP
#define COINS_COLLECTION_SUMMARY_HPP

#include <string>
#include <vector>

namespace coins {

/// Number of coins originating from one country.
struct CountryCount {
  std::string country;
  int coin_count = 0;

  friend bool operator==(const CountryCount&, const CountryCount&) = default;
};

/// A snapshot of the whole collection's headline figures.
struct CollectionSummary {
  int coin_count = 0;

  /// Sum, in EUR, of each coin's latest value estimate. Coins with no estimate
  /// contribute nothing.
  double total_estimate_eur = 0.0;

  /// Coin counts grouped by country, sorted by descending count then country
  /// name.
  std::vector<CountryCount> coins_by_country;

  friend bool operator==(const CollectionSummary&, const CollectionSummary&) = default;
};

}  // namespace coins

#endif  // COINS_COLLECTION_SUMMARY_HPP
