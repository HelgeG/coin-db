#ifndef COINS_I_VALUE_ESTIMATE_REPOSITORY_HPP
#define COINS_I_VALUE_ESTIMATE_REPOSITORY_HPP

#include <expected>
#include <optional>
#include <vector>

#include "coins/id.hpp"
#include "coins/validation.hpp"
#include "coins/value_estimate.hpp"

namespace coins {

/// Persistence boundary for EUR value estimates. Estimates are append-only
/// history: `add` only inserts, never overwrites, so the full record of how a
/// coin's estimated value changed over time is retained. The current value of a
/// coin is its latest estimate (`latest_for_coin`).
class IValueEstimateRepository {
 public:
  virtual ~IValueEstimateRepository() = default;

  /// Validates and appends an estimate for `estimate.coin_id`. Returns the
  /// stored estimate with its assigned id.
  [[nodiscard]] virtual std::expected<ValueEstimate, ValidationErrors> add(
      const ValueEstimate& estimate) = 0;

  /// The estimate history for a coin, oldest first (by `estimated_at`, then id).
  [[nodiscard]] virtual std::vector<ValueEstimate> list_for_coin(Id coin_id) = 0;

  /// The most recent estimate for a coin (latest `estimated_at`, id as
  /// tie-break), or `std::nullopt` if the coin has none.
  [[nodiscard]] virtual std::optional<ValueEstimate> latest_for_coin(Id coin_id) = 0;
};

}  // namespace coins

#endif  // COINS_I_VALUE_ESTIMATE_REPOSITORY_HPP
