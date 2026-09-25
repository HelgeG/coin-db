#include "coins/db/sqlite_value_estimate_repository.hpp"

#include <string_view>
#include <utility>

#include "coins/db/database.hpp"
#include "coins/db/statement.hpp"

namespace coins::db {
namespace {

// Reads a value estimate from the current row of a SELECT with columns
// (id, coin_id, amount_eur, estimated_at, source) in that order.
ValueEstimate map_row(Statement& stmt) {
  ValueEstimate estimate;
  estimate.id = stmt.column_int64(0);
  estimate.coin_id = stmt.column_int64(1);
  estimate.amount_eur = stmt.column_double(2);
  estimate.estimated_at = stmt.column_text(3);
  estimate.source = stmt.column_opt_text(4);
  return estimate;
}

}  // namespace

SqliteValueEstimateRepository::SqliteValueEstimateRepository(Database& db) : db_(db) {}

std::expected<ValueEstimate, ValidationErrors> SqliteValueEstimateRepository::add(
    const ValueEstimate& estimate) {
  if (ValidationResult valid = validate_value_estimate(estimate); !valid) {
    return std::unexpected(std::move(valid).error());
  }

  Statement stmt = db_.prepare(
      "INSERT INTO value_estimate (coin_id, amount_eur, estimated_at, source) "
      "VALUES (?, ?, ?, ?);");
  stmt.bind(1, estimate.coin_id);
  stmt.bind(2, estimate.amount_eur);
  stmt.bind(3, std::string_view{estimate.estimated_at});
  stmt.bind(4, estimate.source);
  (void)stmt.step();

  ValueEstimate stored = estimate;
  stored.id = db_.last_insert_rowid();
  return stored;
}

std::vector<ValueEstimate> SqliteValueEstimateRepository::list_for_coin(Id coin_id) {
  std::vector<ValueEstimate> estimates;
  Statement stmt = db_.prepare(
      "SELECT id, coin_id, amount_eur, estimated_at, source FROM value_estimate "
      "WHERE coin_id = ? ORDER BY estimated_at ASC, id ASC;");
  stmt.bind(1, coin_id);
  while (stmt.step()) {
    estimates.push_back(map_row(stmt));
  }
  return estimates;
}

std::optional<ValueEstimate> SqliteValueEstimateRepository::latest_for_coin(Id coin_id) {
  Statement stmt = db_.prepare(
      "SELECT id, coin_id, amount_eur, estimated_at, source FROM value_estimate "
      "WHERE coin_id = ? ORDER BY estimated_at DESC, id DESC LIMIT 1;");
  stmt.bind(1, coin_id);
  if (!stmt.step()) {
    return std::nullopt;
  }
  return map_row(stmt);
}

}  // namespace coins::db
