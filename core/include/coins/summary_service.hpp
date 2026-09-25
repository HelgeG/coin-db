#ifndef COINS_SUMMARY_SERVICE_HPP
#define COINS_SUMMARY_SERVICE_HPP

#include "coins/collection_summary.hpp"

namespace coins::db {
class Database;
}

namespace coins {

/// Computes collection-wide totals directly from the database. Borrows its
/// `Database`, which must outlive the service.
class SummaryService {
 public:
  explicit SummaryService(db::Database& db);

  /// Builds a `CollectionSummary`: coin count, the EUR total of each coin's
  /// latest estimate, and per-currency face-value totals.
  [[nodiscard]] CollectionSummary summarize();

 private:
  db::Database& db_;
};

}  // namespace coins

#endif  // COINS_SUMMARY_SERVICE_HPP
