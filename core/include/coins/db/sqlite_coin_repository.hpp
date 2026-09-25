#ifndef COINS_DB_SQLITE_COIN_REPOSITORY_HPP
#define COINS_DB_SQLITE_COIN_REPOSITORY_HPP

#include <expected>
#include <optional>
#include <vector>

#include "coins/clock.hpp"
#include "coins/coin.hpp"
#include "coins/coin_query.hpp"
#include "coins/i_coin_repository.hpp"
#include "coins/id.hpp"
#include "coins/validation.hpp"

namespace coins::db {

class Database;

/// SQLite-backed `ICoinRepository`. All access is via parameterized statements.
/// Validation runs before any write; timestamps come from the injected clock.
///
/// Borrows its `Database` and `IClock`; both must outlive the repository
/// (dependency injection, per the SOLID guidance in design.md).
class SqliteCoinRepository final : public ICoinRepository {
 public:
  SqliteCoinRepository(Database& db, const IClock& clock);

  [[nodiscard]] std::expected<Coin, ValidationErrors> create(const Coin& coin) override;
  [[nodiscard]] std::optional<Coin> get(Id id) override;
  [[nodiscard]] std::vector<Coin> list() override;
  [[nodiscard]] std::vector<Coin> search(const CoinQuery& query) override;
  [[nodiscard]] std::expected<bool, ValidationErrors> update(const Coin& coin) override;
  [[nodiscard]] bool remove(Id id) override;

 private:
  Database& db_;
  const IClock& clock_;
};

}  // namespace coins::db

#endif  // COINS_DB_SQLITE_COIN_REPOSITORY_HPP
