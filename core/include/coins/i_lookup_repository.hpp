#ifndef COINS_I_LOOKUP_REPOSITORY_HPP
#define COINS_I_LOOKUP_REPOSITORY_HPP

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "coins/currency_unit.hpp"
#include "coins/id.hpp"
#include "coins/lookup_entry.hpp"
#include "coins/lookup_kind.hpp"

namespace coins {

/// Persistence boundary for the controlled-vocabulary tables (`lookup_entry` /
/// `lookup_name`) and currency units (`currency_unit` / `currency_unit_name`).
/// Storage failures throw; absence is reported via `std::optional`.
class ILookupRepository {
 public:
  virtual ~ILookupRepository() = default;

  // --- Entries ------------------------------------------------------------
  /// Fetches an entry by id (any kind).
  [[nodiscard]] virtual std::optional<LookupEntry> find_by_id(Id id) = 0;

  /// Fetches an entry by its `(kind, code)`.
  [[nodiscard]] virtual std::optional<LookupEntry> find_by_code(LookupKind kind,
                                                                std::string_view code) = 0;

  /// Fetches an entry of `kind` whose name in `lang` equals `name`
  /// (case-insensitive). Used for resolve-or-create de-duplication.
  [[nodiscard]] virtual std::optional<LookupEntry> find_by_name(LookupKind kind,
                                                                std::string_view lang,
                                                                std::string_view name) = 0;

  /// All entries of `kind`, ordered by their display name in `lang`.
  [[nodiscard]] virtual std::vector<LookupEntry> list(LookupKind kind, std::string_view lang) = 0;

  /// Inserts a new entry with the given names. Returns the stored entry (with
  /// its assigned id). The caller ensures `(kind, code)` is unique.
  [[nodiscard]] virtual LookupEntry create(const LookupEntry& entry) = 0;

  /// Sets (inserts or replaces) the `lang` name of an existing entry.
  virtual void set_name(Id entry_id, std::string_view lang, std::string_view name) = 0;

  // --- Currency units -----------------------------------------------------
  /// Fetches a currency unit by id.
  [[nodiscard]] virtual std::optional<CurrencyUnit> find_unit_by_id(Id id) = 0;

  /// Fetches a unit of `currency_id` by its `code`.
  [[nodiscard]] virtual std::optional<CurrencyUnit> find_unit_by_code(Id currency_id,
                                                                      std::string_view code) = 0;

  /// Fetches a unit of `currency_id` whose name in `lang` equals `name`
  /// (case-insensitive).
  [[nodiscard]] virtual std::optional<CurrencyUnit> find_unit_by_name(Id currency_id,
                                                                      std::string_view lang,
                                                                      std::string_view name) = 0;

  /// The major unit of `currency_id`, if any.
  [[nodiscard]] virtual std::optional<CurrencyUnit> major_unit(Id currency_id) = 0;

  /// All units of `currency_id` (major first, then by name).
  [[nodiscard]] virtual std::vector<CurrencyUnit> list_units(Id currency_id) = 0;

  /// Inserts a new currency unit with its names. Returns the stored unit.
  [[nodiscard]] virtual CurrencyUnit create_unit(const CurrencyUnit& unit) = 0;
};

}  // namespace coins

#endif  // COINS_I_LOOKUP_REPOSITORY_HPP
