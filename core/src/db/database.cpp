#include "coins/db/database.hpp"

#include <sqlite3.h>

#include <string>
#include <utility>

#include "coins/db/database_error.hpp"

namespace coins::db {

Database::Database(const std::string& path) {
  const int rc =
      sqlite3_open_v2(path.c_str(), &handle_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
  if (rc != SQLITE_OK) {
    // sqlite3_open_v2 may allocate a handle even on failure; capture the message
    // then release it so the object is not left half-constructed.
    const std::string message =
        handle_ != nullptr ? sqlite3_errmsg(handle_) : "failed to open database";
    sqlite3_close_v2(handle_);
    handle_ = nullptr;
    throw DatabaseError(rc, "open '" + path + "': " + message);
  }
  // Enforce foreign-key constraints (off by default in SQLite) so ON DELETE
  // CASCADE and REFERENCES behave as the schema declares.
  execute("PRAGMA foreign_keys = ON;");
}

Database Database::in_memory() { return Database{":memory:"}; }

Database::Database(Database&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }

Database& Database::operator=(Database&& other) noexcept {
  if (this != &other) {
    sqlite3_close_v2(handle_);
    handle_ = other.handle_;
    other.handle_ = nullptr;
  }
  return *this;
}

Database::~Database() { sqlite3_close_v2(handle_); }

void Database::execute(std::string_view sql) {
  char* error_message = nullptr;
  // sqlite3_exec needs a NUL-terminated string.
  const std::string sql_string{sql};
  const int rc = sqlite3_exec(handle_, sql_string.c_str(), nullptr, nullptr, &error_message);
  if (rc != SQLITE_OK) {
    const std::string message = error_message != nullptr ? error_message : "exec failed";
    sqlite3_free(error_message);
    throw DatabaseError(rc, "execute: " + message);
  }
}

Statement Database::prepare(std::string_view sql) {
  sqlite3_stmt* stmt = nullptr;
  const int rc =
      sqlite3_prepare_v2(handle_, sql.data(), static_cast<int>(sql.size()), &stmt, nullptr);
  if (rc != SQLITE_OK) {
    const std::string message = sqlite3_errmsg(handle_);
    throw DatabaseError(rc, "prepare: " + message);
  }
  return Statement{stmt};
}

std::int64_t Database::last_insert_rowid() const noexcept {
  return sqlite3_last_insert_rowid(handle_);
}

int Database::changes() const noexcept { return sqlite3_changes(handle_); }

}  // namespace coins::db
