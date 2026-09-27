#ifndef COINS_CURRENCY_UNIT_HPP
#define COINS_CURRENCY_UNIT_HPP

#include <map>
#include <string>
#include <string_view>

#include "coins/id.hpp"
#include "coins/lookup_entry.hpp"

namespace coins {

/// A denomination unit of a currency (e.g. krone or øre for NOK), so a coin's
/// face value can be recorded in its natural unit ("50 øre"). Belongs to a
/// currency `lookup_entry`. `minor_per_unit` is how many of the currency's
/// smallest unit this represents (major unit = 100 when 1 major = 100 minor;
/// the minor unit = 1). `is_major` marks the primary unit.
struct CurrencyUnit {
  Id id = kUnsavedId;
  Id currency_id = kUnsavedId;
  std::string code;  // unique within the currency, e.g. "ore"
  int minor_per_unit = 1;
  bool is_major = false;
  std::map<Lang, std::string> names;

  friend bool operator==(const CurrencyUnit&, const CurrencyUnit&) = default;

  /// Display name for `lang`, falling back to English, then any available name,
  /// then the code.
  [[nodiscard]] std::string display_name(std::string_view lang) const;
};

}  // namespace coins

#endif  // COINS_CURRENCY_UNIT_HPP
