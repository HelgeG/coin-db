#ifndef COINS_DB_DATABASE_HPP
#define COINS_DB_DATABASE_HPP

#include <cstdint>
#include <string>
#include <string_view>

#include "coins/db/statement.hpp"

struct sqlite3;

namespace coins::db {

/// RAII owner of a SQLite connection.
///
/// Opens a database file (or an in-memory database) and enables foreign-key
/// enforcement for the connection. All statement execution goes through this
/// type, either via `execute` for simple DDL/pragmas or `prepare` for
/// parameterized queries. Movable, non-copyable.
class Database {
 public:
  /// Opens (creating if necessary) the database at `path`. Use ":memory:" for
  /// an in-memory database. Throws DatabaseError on failure.
  explicit Database(const std::string& path);

  /// Convenience factory for an in-memory database (useful in tests).
  [[nodiscard]] static Database in_memory();

  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;
  Database(Database&& other) noexcept;
  Database& operator=(Database&& other) noexcept;
  ~Database();

  /// Executes one or more semicolon-separated statements with no result rows
  /// (DDL, pragmas, transaction control). Throws DatabaseError on failure.
  void execute(std::string_view sql);

  /// Compiles `sql` into a prepared, parameterized statement.
  [[nodiscard]] Statement prepare(std::string_view sql);

  /// Rowid assigned to the most recent successful INSERT on this connection.
  [[nodiscard]] std::int64_t last_insert_rowid() const noexcept;

  /// Number of rows changed by the most recent INSERT/UPDATE/DELETE statement.
  [[nodiscard]] int changes() const noexcept;

  [[nodiscard]] sqlite3* handle() const noexcept { return handle_; }

 private:
  sqlite3* handle_ = nullptr;
};

}  // namespace coins::db

#endif  // COINS_DB_DATABASE_HPP
