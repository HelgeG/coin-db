#include "coins/db/sqlite_reference_link_repository.hpp"

#include <string_view>
#include <utility>

#include "coins/db/database.hpp"
#include "coins/db/statement.hpp"

namespace coins::db {

SqliteReferenceLinkRepository::SqliteReferenceLinkRepository(Database& db) : db_(db) {}

std::expected<ReferenceLink, ValidationErrors> SqliteReferenceLinkRepository::add(
    const ReferenceLink& link) {
  if (ValidationResult valid = validate_reference_link(link); !valid) {
    return std::unexpected(std::move(valid).error());
  }

  Statement stmt =
      db_.prepare("INSERT INTO reference_link (coin_id, label, url) VALUES (?, ?, ?);");
  stmt.bind(1, link.coin_id);
  stmt.bind(2, std::string_view{link.label});
  stmt.bind(3, std::string_view{link.url});
  (void)stmt.step();

  ReferenceLink stored = link;
  stored.id = db_.last_insert_rowid();
  return stored;
}

std::vector<ReferenceLink> SqliteReferenceLinkRepository::list(Id coin_id) {
  std::vector<ReferenceLink> links;
  Statement stmt = db_.prepare(
      "SELECT id, coin_id, label, url FROM reference_link WHERE coin_id = ? ORDER BY id;");
  stmt.bind(1, coin_id);
  while (stmt.step()) {
    ReferenceLink link;
    link.id = stmt.column_int64(0);
    link.coin_id = stmt.column_int64(1);
    link.label = stmt.column_text(2);
    link.url = stmt.column_text(3);
    links.push_back(std::move(link));
  }
  return links;
}

bool SqliteReferenceLinkRepository::remove(Id id) {
  Statement stmt = db_.prepare("DELETE FROM reference_link WHERE id = ?;");
  stmt.bind(1, id);
  (void)stmt.step();
  return db_.changes() > 0;
}

}  // namespace coins::db
