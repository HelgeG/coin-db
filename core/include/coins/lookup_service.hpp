#ifndef COINS_LOOKUP_SERVICE_HPP
#define COINS_LOOKUP_SERVICE_HPP

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "coins/currency_unit.hpp"
#include "coins/i_lookup_repository.hpp"
#include "coins/id.hpp"
#include "coins/lookup_entry.hpp"
#include "coins/lookup_kind.hpp"

namespace coins {

/// Owns the controlled-vocabulary resolution rules on top of an
/// `ILookupRepository`: display-name resolution, resolve-or-create with
/// case-insensitive de-duplication, code-slug generation, and currency units.
/// Borrows the repository, which must outlive the service.
class LookupService {
 public:
  explicit LookupService(ILookupRepository& repo);

  /// Lists a kind's entries for display in `lang`.
  [[nodiscard]] std::vector<LookupEntry> list(LookupKind kind, std::string_view lang);

  [[nodiscard]] std::optional<LookupEntry> find_by_id(Id id);

  /// Resolves free text to an entry of `kind`: reuses an existing entry whose
  /// `lang` name matches `text` case-insensitively, otherwise creates a new
  /// entry (with a generated code, or the given ISO code for a country/currency
  /// when `text` already looks like a code) carrying `text` as its `lang` name.
  [[nodiscard]] LookupEntry resolve_or_create(LookupKind kind, std::string_view lang,
                                              std::string_view text);

  // --- Currency units -----------------------------------------------------
  [[nodiscard]] std::vector<CurrencyUnit> list_units(Id currency_id);
  [[nodiscard]] std::optional<CurrencyUnit> find_unit_by_id(Id id);
  [[nodiscard]] std::optional<CurrencyUnit> major_unit(Id currency_id);

  /// Resolves free text to a unit of `currency_id`: reuse by `lang` name
  /// (case-insensitive), else create a minor-style unit (minor_per_unit = 1,
  /// not major) with a generated code and `text` as its `lang` name.
  [[nodiscard]] CurrencyUnit resolve_or_create_unit(Id currency_id, std::string_view lang,
                                                    std::string_view text);

 private:
  [[nodiscard]] std::string unique_code(LookupKind kind, std::string_view text);
  [[nodiscard]] std::string unique_unit_code(Id currency_id, std::string_view text);

  ILookupRepository& repo_;
};

/// Turns display text into a lowercase ascii slug (letters/digits, others → '-').
[[nodiscard]] std::string slugify(std::string_view text);

}  // namespace coins

#endif  // COINS_LOOKUP_SERVICE_HPP
