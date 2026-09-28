#ifndef COINS_COLLECTION_IO_HPP
#define COINS_COLLECTION_IO_HPP

#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "coins/id.hpp"
#include "coins/validation.hpp"

namespace coins::db {
class Database;
}

namespace coins {

/// Counts of rows written by a successful import.
struct ImportStats {
  int coins = 0;
  int value_estimates = 0;
  int reference_links = 0;
  int images = 0;

  friend bool operator==(const ImportStats&, const ImportStats&) = default;
};

/// Serializes the entire collection graph (coins with their value estimates,
/// reference links, and image rows) to a pretty-printed JSON string. Images are
/// referenced by their `stored_path`; the image files themselves are backed up
/// separately by copying the image-store directory.
[[nodiscard]] std::string export_json(db::Database& db);

/// Imports a collection from JSON produced by `export_json`. Every coin and
/// child row is validated first; if anything is invalid the import is abandoned
/// and the failures are returned (nothing is written). On success the rows are
/// inserted inside a single transaction with their original ids preserved, so a
/// restore reproduces the collection exactly (and keeps image `stored_path`s,
/// which embed the coin id, valid). Malformed JSON is reported as a validation
/// error. Storage failures roll the transaction back and throw.
[[nodiscard]] std::expected<ImportStats, ValidationErrors> import_json(db::Database& db,
                                                                       std::string_view json_text);

/// Exports a flattened, one-row-per-coin CSV: the coin's columns plus its latest
/// EUR estimate (amount and date), suitable for spreadsheets. Encoded fields
/// (country, denomination, composition, mint, currency, unit) are rendered as
/// their localized display name in `lang` (no codes).
[[nodiscard]] std::string export_csv(db::Database& db, std::string_view lang = "en");

/// As `export_csv`, but restricted to the coins whose ids are in `coin_ids`
/// (e.g. the result of a search). Rows keep the same columns/format and are
/// ordered by coin id regardless of the order of `coin_ids`. An empty `coin_ids`
/// yields a header-only CSV. Callers that want the whole collection use the
/// overload above.
[[nodiscard]] std::string export_csv(db::Database& db, const std::vector<Id>& coin_ids,
                                     std::string_view lang = "en");

}  // namespace coins

#endif  // COINS_COLLECTION_IO_HPP
