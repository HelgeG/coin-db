#ifndef COINS_DB_SQLITE_REFERENCE_LINK_REPOSITORY_HPP
#define COINS_DB_SQLITE_REFERENCE_LINK_REPOSITORY_HPP

#include <expected>
#include <vector>

#include "coins/i_reference_link_repository.hpp"
#include "coins/id.hpp"
#include "coins/reference_link.hpp"
#include "coins/validation.hpp"

namespace coins::db {

class Database;

/// SQLite-backed `IReferenceLinkRepository`. Parameterized access throughout.
/// Borrows its `Database`, which must outlive the repository.
class SqliteReferenceLinkRepository final : public IReferenceLinkRepository {
 public:
  explicit SqliteReferenceLinkRepository(Database& db);

  [[nodiscard]] std::expected<ReferenceLink, ValidationErrors> add(
      const ReferenceLink& link) override;
  [[nodiscard]] std::vector<ReferenceLink> list(Id coin_id) override;
  [[nodiscard]] bool remove(Id id) override;

 private:
  Database& db_;
};

}  // namespace coins::db

#endif  // COINS_DB_SQLITE_REFERENCE_LINK_REPOSITORY_HPP
