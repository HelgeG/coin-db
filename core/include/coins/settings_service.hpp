#ifndef COINS_SETTINGS_SERVICE_HPP
#define COINS_SETTINGS_SERVICE_HPP

#include <optional>

#include "coins/id.hpp"
#include "coins/lookup_entry.hpp"

namespace coins::db {
class Database;
}

namespace coins {

class ILookupRepository;

/// Reads and writes collection-wide settings (the `app_setting` table). Today
/// the only setting is the base currency — the `currency` lookup entry that all
/// value estimates and acquisition prices are expressed in (no conversion).
/// Borrows its `Database` and `ILookupRepository`, which must outlive it.
class SettingsService {
 public:
  SettingsService(db::Database& db, ILookupRepository& lookups);

  /// The collection's base currency entry. Falls back to the EUR currency entry
  /// (and persists it) when unset; returns `std::nullopt` only if no currency
  /// vocabulary exists at all.
  [[nodiscard]] std::optional<LookupEntry> base_currency();

  /// Sets the base currency to `currency_id`. Returns false if the id is not a
  /// `currency` lookup entry (the setting is left unchanged).
  [[nodiscard]] bool set_base_currency(Id currency_id);

 private:
  [[nodiscard]] std::optional<Id> read_base_currency_id();
  void write_base_currency_id(Id id);

  db::Database& db_;
  ILookupRepository& lookups_;
};

}  // namespace coins

#endif  // COINS_SETTINGS_SERVICE_HPP
