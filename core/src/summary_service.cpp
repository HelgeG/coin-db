#include "coins/summary_service.hpp"

#include <algorithm>
#include <string>

#include "coins/db/database.hpp"
#include "coins/db/statement.hpp"

namespace coins {

using db::Statement;

namespace {

/// Runs a two-column `SELECT label, COUNT(*)` grouping query and collects the
/// rows into buckets, rendering a NULL label as `null_label`. `lang` is bound to
/// every `?` placeholder in `sql` (queries here use it 0 or more times for the
/// localized-name join). Buckets are then sorted by descending count, then by
/// rendered label ascending — sorting in C++ (rather than SQL) keeps the
/// ordering consistent for the substituted null-label buckets too.
void collect_label_counts(db::Database& db, const std::string& sql, std::vector<SummaryBucket>& out,
                          std::string_view null_label, std::string_view lang, int lang_binds) {
  Statement stmt = db.prepare(sql);
  for (int i = 1; i <= lang_binds; ++i) {
    stmt.bind(i, lang);
  }
  while (stmt.step()) {
    std::string label = stmt.column_opt_text(0).value_or(std::string(null_label));
    out.push_back({std::move(label), static_cast<int>(stmt.column_int64(1))});
  }
  std::sort(out.begin(), out.end(), [null_label](const SummaryBucket& a, const SummaryBucket& b) {
    if (a.coin_count != b.coin_count) return a.coin_count > b.coin_count;
    // Within a tie, keep the substituted null bucket (e.g. "(ungraded)") last.
    const bool a_null = a.label == null_label;
    const bool b_null = b.label == null_label;
    if (a_null != b_null) return b_null;
    return a.label < b.label;
  });
}

void fill_breakdown(db::Database& db, SummaryBreakdown& breakdown, std::string_view lang) {
  switch (breakdown.type) {
    case SummaryType::TotalValue:
      // Headline total only; no buckets.
      break;
    case SummaryType::ByCountry:
      // Group by the country lookup entry; label with its localized name
      // (falling back to the code). The `?` binds the active language.
      collect_label_counts(db,
                           "SELECT COALESCE(n.name, e.code) AS label, COUNT(*) FROM coin c "
                           "JOIN lookup_entry e ON e.id = c.country_id "
                           "LEFT JOIN lookup_name n ON n.entry_id = e.id AND n.lang = ? "
                           "GROUP BY e.id ORDER BY COUNT(*) DESC, label ASC;",
                           breakdown.buckets, "(unknown)", lang, 1);
      break;
    case SummaryType::ByDecade:
      // Group by decade of the coin's starting year, e.g. 1963 -> "1960s".
      collect_label_counts(db,
                           "SELECT CAST((year_from / 10) * 10 AS TEXT) || 's', COUNT(*) FROM coin "
                           "GROUP BY (year_from / 10) * 10 "
                           "ORDER BY COUNT(*) DESC, (year_from / 10) * 10 ASC;",
                           breakdown.buckets, "(unknown)", lang, 0);
      break;
    case SummaryType::ByGrade:
      // NULLIF collapses empty labels to NULL so they land in the "(ungraded)"
      // bucket rather than an empty-string one.
      collect_label_counts(db,
                           "SELECT NULLIF(grade_label, ''), COUNT(*) FROM coin "
                           "GROUP BY NULLIF(grade_label, '') "
                           "ORDER BY COUNT(*) DESC, grade_label ASC;",
                           breakdown.buckets, "(ungraded)", lang, 0);
      break;
    case SummaryType::ByMetal:
      // Group by the composition lookup entry; label with its localized name.
      // Coins without a composition fall into the "(unknown)" bucket.
      collect_label_counts(db,
                           "SELECT COALESCE(n.name, e.code) AS label, COUNT(*) FROM coin c "
                           "LEFT JOIN lookup_entry e ON e.id = c.composition_id "
                           "LEFT JOIN lookup_name n ON n.entry_id = e.id AND n.lang = ? "
                           "GROUP BY c.composition_id ORDER BY COUNT(*) DESC, label ASC;",
                           breakdown.buckets, "(unknown)", lang, 1);
      break;
  }
}

}  // namespace

SummaryService::SummaryService(db::Database& db) : db_(db) {}

CollectionSummary SummaryService::summarize(SummaryType type, std::string_view lang) {
  CollectionSummary summary;
  summary.breakdown.type = type;

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
        "SELECT COALESCE(SUM(amount), 0.0) FROM ("
        "  SELECT amount, ROW_NUMBER() OVER "
        "    (PARTITION BY coin_id ORDER BY estimated_at DESC, id DESC) AS rn"
        "  FROM value_estimate"
        ") WHERE rn = 1;");
    if (stmt.step()) {
      summary.total_estimate_eur = stmt.column_double(0);
    }
  }

  fill_breakdown(db_, summary.breakdown, lang);
  return summary;
}

}  // namespace coins
