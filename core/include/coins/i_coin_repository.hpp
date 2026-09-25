#ifndef COINS_I_COIN_REPOSITORY_HPP
#define COINS_I_COIN_REPOSITORY_HPP

#include <expected>
#include <optional>
#include <vector>

#include "coins/coin.hpp"
#include "coins/coin_query.hpp"
#include "coins/id.hpp"
#include "coins/validation.hpp"

namespace coins {

/// Persistence boundary for coins. High-level logic depends on this abstraction,
/// not on SQLite (dependency inversion); `SqliteCoinRepository` is one
/// implementation, and tests can substitute fakes.
///
/// Failure conventions:
///   - Domain validation failures are returned via `std::expected` (expected).
///   - "Not found" is absence: `std::optional` from `get`, `bool` from
///     `update`/`remove` — not an error.
///   - Storage failures throw `coins::db::DatabaseError` (exceptional).
class ICoinRepository {
 public:
  virtual ~ICoinRepository() = default;

  /// Validates and inserts a new coin. On success returns the stored coin with
  /// its assigned id and `created_at`/`updated_at` populated.
  [[nodiscard]] virtual std::expected<Coin, ValidationErrors> create(const Coin& coin) = 0;

  /// Fetches a coin by id, or `std::nullopt` if none exists.
  [[nodiscard]] virtual std::optional<Coin> get(Id id) = 0;

  /// Returns all coins (ordered by id).
  [[nodiscard]] virtual std::vector<Coin> list() = 0;

  /// Returns coins matching `query`, filtered and sorted per its criteria.
  [[nodiscard]] virtual std::vector<Coin> search(const CoinQuery& query) = 0;

  /// Validates and updates an existing coin (matched by `coin.id`), refreshing
  /// `updated_at`. Returns whether a coin with that id existed; validation
  /// failures are returned as `unexpected`.
  [[nodiscard]] virtual std::expected<bool, ValidationErrors> update(const Coin& coin) = 0;

  /// Deletes a coin by id, cascading to its estimates, links, and images.
  /// Returns whether a coin was removed.
  [[nodiscard]] virtual bool remove(Id id) = 0;
};

}  // namespace coins

#endif  // COINS_I_COIN_REPOSITORY_HPP
