#include "coins/db/sqlite_lookup_repository.hpp"

#include <string>
#include <utility>

#include "coins/db/database.hpp"
#include "coins/db/statement.hpp"

namespace coins::db {
namespace {

void load_entry_names(Database& db, LookupEntry& entry) {
  Statement stmt = db.prepare("SELECT lang, name FROM lookup_name WHERE entry_id = ?;");
  stmt.bind(1, entry.id);
  while (stmt.step()) {
    entry.names[stmt.column_text(0)] = stmt.column_text(1);
  }
}

void load_unit_names(Database& db, CurrencyUnit& unit) {
  Statement stmt = db.prepare("SELECT lang, name FROM currency_unit_name WHERE unit_id = ?;");
  stmt.bind(1, unit.id);
  while (stmt.step()) {
    unit.names[stmt.column_text(0)] = stmt.column_text(1);
  }
}

CurrencyUnit read_unit(Database& db, Statement& stmt) {
  CurrencyUnit unit;
  unit.id = stmt.column_int64(0);
  unit.currency_id = stmt.column_int64(1);
  unit.code = stmt.column_text(2);
  unit.minor_per_unit = static_cast<int>(stmt.column_int64(3));
  unit.is_major = stmt.column_int64(4) != 0;
  load_unit_names(db, unit);
  return unit;
}

}  // namespace

SqliteLookupRepository::SqliteLookupRepository(Database& db) : db_(db) {}

std::optional<LookupEntry> SqliteLookupRepository::find_by_id(Id id) {
  Statement stmt = db_.prepare("SELECT id, kind, code FROM lookup_entry WHERE id = ?;");
  stmt.bind(1, id);
  if (!stmt.step()) {
    return std::nullopt;
  }
  LookupEntry entry;
  entry.id = stmt.column_int64(0);
  entry.kind = lookup_kind_from_string(stmt.column_text(1)).value_or(LookupKind::Country);
  entry.code = stmt.column_text(2);
  load_entry_names(db_, entry);
  return entry;
}

std::optional<LookupEntry> SqliteLookupRepository::find_by_code(LookupKind kind,
                                                                std::string_view code) {
  Statement stmt = db_.prepare("SELECT id FROM lookup_entry WHERE kind = ? AND code = ?;");
  stmt.bind(1, to_string(kind));
  stmt.bind(2, code);
  if (!stmt.step()) {
    return std::nullopt;
  }
  return find_by_id(stmt.column_int64(0));
}

std::optional<LookupEntry> SqliteLookupRepository::find_by_name(LookupKind kind,
                                                                std::string_view lang,
                                                                std::string_view name) {
  Statement stmt = db_.prepare(
      "SELECT e.id FROM lookup_entry e JOIN lookup_name n ON n.entry_id = e.id "
      "WHERE e.kind = ? AND n.lang = ? AND n.name = ? COLLATE NOCASE LIMIT 1;");
  stmt.bind(1, to_string(kind));
  stmt.bind(2, lang);
  stmt.bind(3, name);
  if (!stmt.step()) {
    return std::nullopt;
  }
  return find_by_id(stmt.column_int64(0));
}

std::vector<LookupEntry> SqliteLookupRepository::list(LookupKind kind, std::string_view lang) {
  // Order by the localized name when present, else by code, so the list reads
  // naturally in the active language.
  std::vector<LookupEntry> out;
  Statement stmt = db_.prepare(
      "SELECT e.id FROM lookup_entry e "
      "LEFT JOIN lookup_name n ON n.entry_id = e.id AND n.lang = ? "
      "WHERE e.kind = ? ORDER BY COALESCE(n.name, e.code) COLLATE NOCASE;");
  stmt.bind(1, lang);
  stmt.bind(2, to_string(kind));
  while (stmt.step()) {
    if (auto entry = find_by_id(stmt.column_int64(0))) {
      out.push_back(std::move(*entry));
    }
  }
  return out;
}

LookupEntry SqliteLookupRepository::create(const LookupEntry& entry) {
  Statement ins = db_.prepare("INSERT INTO lookup_entry (kind, code) VALUES (?, ?);");
  ins.bind(1, to_string(entry.kind));
  ins.bind(2, std::string_view{entry.code});
  (void)ins.step();

  LookupEntry stored = entry;
  stored.id = db_.last_insert_rowid();
  for (const auto& [lang, name] : stored.names) {
    set_name(stored.id, lang, name);
  }
  return stored;
}

void SqliteLookupRepository::set_name(Id entry_id, std::string_view lang, std::string_view name) {
  Statement stmt = db_.prepare(
      "INSERT INTO lookup_name (entry_id, lang, name) VALUES (?, ?, ?) "
      "ON CONFLICT(entry_id, lang) DO UPDATE SET name = excluded.name;");
  stmt.bind(1, entry_id);
  stmt.bind(2, lang);
  stmt.bind(3, name);
  (void)stmt.step();
}

std::optional<CurrencyUnit> SqliteLookupRepository::find_unit_by_id(Id id) {
  Statement stmt = db_.prepare(
      "SELECT id, currency_id, code, minor_per_unit, is_major FROM currency_unit WHERE id = ?;");
  stmt.bind(1, id);
  if (!stmt.step()) {
    return std::nullopt;
  }
  return read_unit(db_, stmt);
}

std::optional<CurrencyUnit> SqliteLookupRepository::find_unit_by_code(Id currency_id,
                                                                      std::string_view code) {
  Statement stmt = db_.prepare(
      "SELECT id, currency_id, code, minor_per_unit, is_major FROM currency_unit "
      "WHERE currency_id = ? AND code = ?;");
  stmt.bind(1, currency_id);
  stmt.bind(2, code);
  if (!stmt.step()) {
    return std::nullopt;
  }
  return read_unit(db_, stmt);
}

std::optional<CurrencyUnit> SqliteLookupRepository::find_unit_by_name(Id currency_id,
                                                                      std::string_view lang,
                                                                      std::string_view name) {
  Statement stmt = db_.prepare(
      "SELECT u.id, u.currency_id, u.code, u.minor_per_unit, u.is_major "
      "FROM currency_unit u JOIN currency_unit_name n ON n.unit_id = u.id "
      "WHERE u.currency_id = ? AND n.lang = ? AND n.name = ? COLLATE NOCASE LIMIT 1;");
  stmt.bind(1, currency_id);
  stmt.bind(2, lang);
  stmt.bind(3, name);
  if (!stmt.step()) {
    return std::nullopt;
  }
  return read_unit(db_, stmt);
}

std::optional<CurrencyUnit> SqliteLookupRepository::major_unit(Id currency_id) {
  Statement stmt = db_.prepare(
      "SELECT id, currency_id, code, minor_per_unit, is_major FROM currency_unit "
      "WHERE currency_id = ? AND is_major = 1 LIMIT 1;");
  stmt.bind(1, currency_id);
  if (!stmt.step()) {
    return std::nullopt;
  }
  return read_unit(db_, stmt);
}

std::vector<CurrencyUnit> SqliteLookupRepository::list_units(Id currency_id) {
  std::vector<CurrencyUnit> out;
  Statement stmt = db_.prepare(
      "SELECT id, currency_id, code, minor_per_unit, is_major FROM currency_unit "
      "WHERE currency_id = ? ORDER BY is_major DESC, code;");
  stmt.bind(1, currency_id);
  while (stmt.step()) {
    out.push_back(read_unit(db_, stmt));
  }
  return out;
}

CurrencyUnit SqliteLookupRepository::create_unit(const CurrencyUnit& unit) {
  Statement ins = db_.prepare(
      "INSERT INTO currency_unit (currency_id, code, minor_per_unit, is_major) "
      "VALUES (?, ?, ?, ?);");
  ins.bind(1, unit.currency_id);
  ins.bind(2, std::string_view{unit.code});
  ins.bind(3, unit.minor_per_unit);
  ins.bind(4, unit.is_major ? 1 : 0);
  (void)ins.step();

  CurrencyUnit stored = unit;
  stored.id = db_.last_insert_rowid();
  for (const auto& [lang, name] : stored.names) {
    Statement n =
        db_.prepare("INSERT INTO currency_unit_name (unit_id, lang, name) VALUES (?, ?, ?);");
    n.bind(1, stored.id);
    n.bind(2, std::string_view{lang});
    n.bind(3, std::string_view{name});
    (void)n.step();
  }
  return stored;
}

}  // namespace coins::db
