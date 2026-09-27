#ifndef COINS_LOOKUP_KIND_HPP
#define COINS_LOOKUP_KIND_HPP

#include <array>
#include <optional>
#include <string_view>

namespace coins {

/// The controlled-vocabulary fields that are stored as shared, localized lookup
/// entries rather than free text (see design.md, "Controlled Vocabularies").
enum class LookupKind {
  Country,       ///< ISO 3166-1 alpha-2 (or ISO 3166-3 / generated for historical).
  Denomination,  ///< Named piece (e.g. Speciedaler); app-generated code.
  Composition,   ///< Metal/alloy; app-generated code.
  Mint,          ///< Minting facility; app-generated code.
  Currency,      ///< ISO 4217 (current or historical) or generated code.
};

/// Stable wire/CLI key for a lookup kind (e.g. "country").
[[nodiscard]] constexpr std::string_view to_string(LookupKind kind) noexcept {
  switch (kind) {
    case LookupKind::Country:
      return "country";
    case LookupKind::Denomination:
      return "denomination";
    case LookupKind::Composition:
      return "composition";
    case LookupKind::Mint:
      return "mint";
    case LookupKind::Currency:
      return "currency";
  }
  return "country";
}

/// All lookup kinds, in a stable order.
inline constexpr std::array<LookupKind, 5> kAllLookupKinds = {
    LookupKind::Country, LookupKind::Denomination, LookupKind::Composition, LookupKind::Mint,
    LookupKind::Currency};

/// Parses a wire/CLI key into a `LookupKind`. Returns `std::nullopt` for an
/// unknown key.
[[nodiscard]] constexpr std::optional<LookupKind> lookup_kind_from_string(
    std::string_view key) noexcept {
  for (const LookupKind kind : kAllLookupKinds) {
    if (to_string(kind) == key) {
      return kind;
    }
  }
  return std::nullopt;
}

}  // namespace coins

#endif  // COINS_LOOKUP_KIND_HPP
