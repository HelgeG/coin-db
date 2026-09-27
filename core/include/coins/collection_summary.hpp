#ifndef COINS_COLLECTION_SUMMARY_HPP
#define COINS_COLLECTION_SUMMARY_HPP

#include <string>
#include <vector>

#include "coins/summary_type.hpp"

namespace coins {

/// One row of a summary breakdown: a group label and how many coins fall in it.
/// For example `{"Norway", 12}` under a by-country breakdown.
struct SummaryBucket {
  std::string label;
  int coin_count = 0;

  friend bool operator==(const SummaryBucket&, const SummaryBucket&) = default;
};

/// A selected breakdown: its type plus the ordered group buckets. `TotalValue`
/// carries no buckets (the headline total is the whole story).
struct SummaryBreakdown {
  SummaryType type = kDefaultSummaryType;
  std::vector<SummaryBucket> buckets;

  friend bool operator==(const SummaryBreakdown&, const SummaryBreakdown&) = default;
};

/// A snapshot of the collection's headline figures plus one selected breakdown.
struct CollectionSummary {
  int coin_count = 0;

  /// Sum, in EUR, of each coin's latest value estimate. Coins with no estimate
  /// contribute nothing.
  double total_estimate_eur = 0.0;

  /// The selected breakdown (buckets sorted by descending count then label).
  SummaryBreakdown breakdown;

  friend bool operator==(const CollectionSummary&, const CollectionSummary&) = default;
};

}  // namespace coins

#endif  // COINS_COLLECTION_SUMMARY_HPP
