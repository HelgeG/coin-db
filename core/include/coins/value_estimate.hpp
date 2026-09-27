#ifndef COINS_VALUE_ESTIMATE_HPP
#define COINS_VALUE_ESTIMATE_HPP

#include <optional>
#include <string>

#include "coins/id.hpp"

namespace coins {

/// A single value estimate for a coin: one row of the `value_estimate` table.
/// Estimates are append-only history; the coin's current value is the estimate
/// with the most recent `estimated_at`. Amounts are in the collection's base
/// currency (see `SettingsService`); no conversion is performed.
struct ValueEstimate {
  Id id = kUnsavedId;
  Id coin_id = kUnsavedId;

  double amount = 0.0;                // in the collection base currency
  std::string estimated_at;           // ISO 8601 date
  std::optional<std::string> source;  // how the estimate was derived

  friend bool operator==(const ValueEstimate&, const ValueEstimate&) = default;
};

}  // namespace coins

#endif  // COINS_VALUE_ESTIMATE_HPP
