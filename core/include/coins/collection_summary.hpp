#ifndef COINS_COLLECTION_SUMMARY_HPP
#define COINS_COLLECTION_SUMMARY_HPP

#include <string>
#include <vector>

namespace coins {

/// Total face value of coins sharing one denomination currency. Face values are
/// never converted between currencies, so they are reported per currency.
struct FaceValueTotal {
  std::string currency;  // ISO 4217 code, e.g. "NOK"
  double total_face_value = 0.0;

  friend bool operator==(const FaceValueTotal&, const FaceValueTotal&) = default;
};

/// A snapshot of the whole collection's headline figures.
struct CollectionSummary {
  int coin_count = 0;

  /// Sum, in EUR, of each coin's latest value estimate. Coins with no estimate
  /// contribute nothing.
  double total_estimate_eur = 0.0;

  /// Face-value totals grouped by the coins' own currencies, sorted by currency.
  /// Only coins that have both a currency and a face value are included.
  std::vector<FaceValueTotal> face_value_by_currency;

  friend bool operator==(const CollectionSummary&, const CollectionSummary&) = default;
};

}  // namespace coins

#endif  // COINS_COLLECTION_SUMMARY_HPP
