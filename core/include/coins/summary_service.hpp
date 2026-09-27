#ifndef COINS_SUMMARY_SERVICE_HPP
#define COINS_SUMMARY_SERVICE_HPP

#include "coins/collection_summary.hpp"
#include "coins/summary_type.hpp"

namespace coins::db {
class Database;
}

namespace coins {

/// Computes collection-wide totals directly from the database. Borrows its
/// `Database`, which must outlive the service.
class SummaryService {
 public:
  explicit SummaryService(db::Database& db);

  /// Builds a `CollectionSummary`: the coin count, the EUR total of each coin's
  /// latest estimate (always computed), and the selected `type`'s breakdown
  /// buckets. Buckets are sorted by descending count then label; `TotalValue`
  /// produces no buckets.
  [[nodiscard]] CollectionSummary summarize(SummaryType type = kDefaultSummaryType);

 private:
  db::Database& db_;
};

}  // namespace coins

#endif  // COINS_SUMMARY_SERVICE_HPP
