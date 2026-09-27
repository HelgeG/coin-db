#include "coins/settings_service.hpp"

#include <string>

#include "coins/db/database.hpp"
#include "coins/db/statement.hpp"
#include "coins/i_lookup_repository.hpp"
#include "coins/lookup_kind.hpp"

namespace coins {

namespace {
constexpr std::string_view kBaseCurrencyKey = "base_currency_id";
}

SettingsService::SettingsService(db::Database& db, ILookupRepository& lookups)
    : db_(db), lookups_(lookups) {}

std::optional<Id> SettingsService::read_base_currency_id() {
  db::Statement stmt = db_.prepare("SELECT value FROM app_setting WHERE key = ?;");
  stmt.bind(1, kBaseCurrencyKey);
  if (!stmt.step()) {
    return std::nullopt;
  }
  const std::optional<std::string> value = stmt.column_opt_text(0);
  if (!value || value->empty()) {
    return std::nullopt;
  }
  try {
    return static_cast<Id>(std::stoll(*value));
  } catch (...) {
    return std::nullopt;
  }
}

void SettingsService::write_base_currency_id(Id id) {
  db::Statement stmt = db_.prepare(
      "INSERT INTO app_setting (key, value) VALUES (?, ?) "
      "ON CONFLICT(key) DO UPDATE SET value = excluded.value;");
  stmt.bind(1, kBaseCurrencyKey);
  stmt.bind(2, std::to_string(id));
  (void)stmt.step();
}

std::optional<LookupEntry> SettingsService::base_currency() {
  if (const std::optional<Id> id = read_base_currency_id()) {
    if (auto entry = lookups_.find_by_id(*id); entry && entry->kind == LookupKind::Currency) {
      return entry;
    }
  }
  // Unset or dangling: fall back to EUR and persist it.
  if (auto eur = lookups_.find_by_code(LookupKind::Currency, "EUR")) {
    write_base_currency_id(eur->id);
    return eur;
  }
  return std::nullopt;
}

bool SettingsService::set_base_currency(Id currency_id) {
  const auto entry = lookups_.find_by_id(currency_id);
  if (!entry || entry->kind != LookupKind::Currency) {
    return false;
  }
  write_base_currency_id(currency_id);
  return true;
}

}  // namespace coins
