#include "coins/summary_service.hpp"

#include "coins/db/database.hpp"
#include "coins/db/statement.hpp"

namespace coins {

using db::Statement;

SummaryService::SummaryService(db::Database& db) : db_(db) {}

CollectionSummary SummaryService::summarize() {
  CollectionSummary summary;

  {
    Statement stmt = db_.prepare("SELECT COUNT(*) FROM coin;");
    if (stmt.step()) {
      summary.coin_count = static_cast<int>(stmt.column_int64(0));
    }
  }

  {
    // Sum the latest estimate per coin. ROW_NUMBER picks the most recent row
    // (estimated_at, then id as tie-break) within each coin's estimates.
    Statement stmt = db_.prepare(
        "SELECT COALESCE(SUM(amount_eur), 0.0) FROM ("
        "  SELECT amount_eur, ROW_NUMBER() OVER "
        "    (PARTITION BY coin_id ORDER BY estimated_at DESC, id DESC) AS rn"
        "  FROM value_estimate"
        ") WHERE rn = 1;");
    if (stmt.step()) {
      summary.total_estimate_eur = stmt.column_double(0);
    }
  }

  {
    // Count coins per country, most numerous first, then alphabetically.
    Statement stmt = db_.prepare(
        "SELECT country, COUNT(*) FROM coin "
        "GROUP BY country ORDER BY COUNT(*) DESC, country ASC;");
    while (stmt.step()) {
      summary.coins_by_country.push_back(
          {stmt.column_text(0), static_cast<int>(stmt.column_int64(1))});
    }
  }

  return summary;
}

}  // namespace coins
