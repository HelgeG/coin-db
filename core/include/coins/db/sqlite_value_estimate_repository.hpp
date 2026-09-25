#ifndef COINS_DB_SQLITE_VALUE_ESTIMATE_REPOSITORY_HPP
#define COINS_DB_SQLITE_VALUE_ESTIMATE_REPOSITORY_HPP

#include <expected>
#include <optional>
#include <vector>

#include "coins/i_value_estimate_repository.hpp"
#include "coins/id.hpp"
#include "coins/validation.hpp"
#include "coins/value_estimate.hpp"

namespace coins::db {

class Database;

/// SQLite-backed `IValueEstimateRepository`. Parameterized access throughout.
/// Borrows its `Database`, which must outlive the repository.
class SqliteValueEstimateRepository final : public IValueEstimateRepository {
 public:
  explicit SqliteValueEstimateRepository(Database& db);

  [[nodiscard]] std::expected<ValueEstimate, ValidationErrors> add(
      const ValueEstimate& estimate) override;
  [[nodiscard]] std::vector<ValueEstimate> list_for_coin(Id coin_id) override;
  [[nodiscard]] std::optional<ValueEstimate> latest_for_coin(Id coin_id) override;

 private:
  Database& db_;
};

}  // namespace coins::db

#endif  // COINS_DB_SQLITE_VALUE_ESTIMATE_REPOSITORY_HPP
