#ifndef COINS_DB_SQLITE_LOOKUP_REPOSITORY_HPP
#define COINS_DB_SQLITE_LOOKUP_REPOSITORY_HPP

#include <optional>
#include <string_view>
#include <vector>

#include "coins/currency_unit.hpp"
#include "coins/i_lookup_repository.hpp"
#include "coins/id.hpp"
#include "coins/lookup_entry.hpp"
#include "coins/lookup_kind.hpp"

namespace coins::db {

class Database;

/// SQLite-backed `ILookupRepository`. Borrows its `Database`, which must
/// outlive the repository. All access is parameterized.
class SqliteLookupRepository final : public ILookupRepository {
 public:
  explicit SqliteLookupRepository(Database& db);

  [[nodiscard]] std::optional<LookupEntry> find_by_id(Id id) override;
  [[nodiscard]] std::optional<LookupEntry> find_by_code(LookupKind kind,
                                                        std::string_view code) override;
  [[nodiscard]] std::optional<LookupEntry> find_by_name(LookupKind kind, std::string_view lang,
                                                        std::string_view name) override;
  [[nodiscard]] std::vector<LookupEntry> list(LookupKind kind, std::string_view lang) override;
  [[nodiscard]] LookupEntry create(const LookupEntry& entry) override;
  void set_name(Id entry_id, std::string_view lang, std::string_view name) override;

  [[nodiscard]] std::optional<CurrencyUnit> find_unit_by_id(Id id) override;
  [[nodiscard]] std::optional<CurrencyUnit> find_unit_by_code(Id currency_id,
                                                              std::string_view code) override;
  [[nodiscard]] std::optional<CurrencyUnit> find_unit_by_name(Id currency_id, std::string_view lang,
                                                              std::string_view name) override;
  [[nodiscard]] std::optional<CurrencyUnit> major_unit(Id currency_id) override;
  [[nodiscard]] std::vector<CurrencyUnit> list_units(Id currency_id) override;
  [[nodiscard]] CurrencyUnit create_unit(const CurrencyUnit& unit) override;

 private:
  Database& db_;
};

}  // namespace coins::db

#endif  // COINS_DB_SQLITE_LOOKUP_REPOSITORY_HPP
