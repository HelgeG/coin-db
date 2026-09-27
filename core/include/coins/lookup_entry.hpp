#ifndef COINS_LOOKUP_ENTRY_HPP
#define COINS_LOOKUP_ENTRY_HPP

#include <map>
#include <string>
#include <string_view>

#include "coins/id.hpp"
#include "coins/lookup_kind.hpp"

namespace coins {

/// Language code for a localized display name, e.g. "en" or "nb". English
/// ("en") is the fallback language.
using Lang = std::string;

inline constexpr std::string_view kFallbackLang = "en";

/// A shared, localized controlled-vocabulary entry: one `lookup_entry` row plus
/// its per-language names. The `code` is stable and never localized; `names`
/// maps a language code to its display name.
struct LookupEntry {
  Id id = kUnsavedId;
  LookupKind kind = LookupKind::Country;
  std::string code;
  std::map<Lang, std::string> names;

  friend bool operator==(const LookupEntry&, const LookupEntry&) = default;

  /// Display name for `lang`, falling back to English, then any available name,
  /// then the code. Never returns empty for a well-formed entry.
  [[nodiscard]] std::string display_name(std::string_view lang) const;
};

}  // namespace coins

#endif  // COINS_LOOKUP_ENTRY_HPP
