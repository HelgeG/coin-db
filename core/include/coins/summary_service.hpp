#ifndef COINS_SUMMARY_SERVICE_HPP
#define COINS_SUMMARY_SERVICE_HPP

#include <string_view>

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
  /// produces no buckets. For lookup-based breakdowns (by_country, by_metal) the
  /// labels are the entries' display names in `lang` (English fallback).
  [[nodiscard]] CollectionSummary summarize(SummaryType type = kDefaultSummaryType,
                                            std::string_view lang = "en");

 private:
  db::Database& db_;
};

}  // namespace coins

#endif  // COINS_SUMMARY_SERVICE_HPP
