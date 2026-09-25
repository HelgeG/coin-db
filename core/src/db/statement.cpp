#include "coins/db/statement.hpp"

#include <sqlite3.h>

#include <utility>

#include "coins/db/database_error.hpp"

namespace coins::db {
namespace {

/// Builds a DatabaseError from the connection that owns `stmt`, tagging it with
/// the given SQLite result code.
[[noreturn]] void throw_stmt_error(sqlite3_stmt* stmt, int code, const char* context) {
  sqlite3* db = sqlite3_db_handle(stmt);
  const char* message = db != nullptr ? sqlite3_errmsg(db) : "unknown error";
  throw DatabaseError(code, std::string{context} + ": " + message);
}

}  // namespace

Statement::Statement(sqlite3_stmt* stmt) noexcept : stmt_(stmt) {}

Statement::Statement(Statement&& other) noexcept : stmt_(other.stmt_) { other.stmt_ = nullptr; }

Statement& Statement::operator=(Statement&& other) noexcept {
  if (this != &other) {
    sqlite3_finalize(stmt_);
    stmt_ = other.stmt_;
    other.stmt_ = nullptr;
  }
  return *this;
}

Statement::~Statement() { sqlite3_finalize(stmt_); }

void Statement::bind(int index, std::int64_t value) {
  const int rc = sqlite3_bind_int64(stmt_, index, value);
  if (rc != SQLITE_OK) {
    throw_stmt_error(stmt_, rc, "bind int64");
  }
}

void Statement::bind(int index, int value) { bind(index, static_cast<std::int64_t>(value)); }

void Statement::bind(int index, double value) {
  const int rc = sqlite3_bind_double(stmt_, index, value);
  if (rc != SQLITE_OK) {
    throw_stmt_error(stmt_, rc, "bind double");
  }
}

void Statement::bind(int index, std::string_view value) {
  // SQLITE_TRANSIENT: SQLite copies the bytes, so `value` need not outlive the
  // call. Length is bounded by SQLite's own int-sized limits.
  const int rc = sqlite3_bind_text(stmt_, index, value.data(), static_cast<int>(value.size()),
                                   SQLITE_TRANSIENT);
  if (rc != SQLITE_OK) {
    throw_stmt_error(stmt_, rc, "bind text");
  }
}

void Statement::bind(int index, std::nullptr_t) {
  const int rc = sqlite3_bind_null(stmt_, index);
  if (rc != SQLITE_OK) {
    throw_stmt_error(stmt_, rc, "bind null");
  }
}

bool Statement::step() {
  const int rc = sqlite3_step(stmt_);
  if (rc == SQLITE_ROW) {
    return true;
  }
  if (rc == SQLITE_DONE) {
    return false;
  }
  throw_stmt_error(stmt_, rc, "step");
}

void Statement::reset() {
  const int rc = sqlite3_reset(stmt_);
  if (rc != SQLITE_OK) {
    throw_stmt_error(stmt_, rc, "reset");
  }
}

int Statement::column_count() const noexcept { return sqlite3_column_count(stmt_); }

bool Statement::is_null(int index) const {
  return sqlite3_column_type(stmt_, index) == SQLITE_NULL;
}

std::int64_t Statement::column_int64(int index) const { return sqlite3_column_int64(stmt_, index); }

double Statement::column_double(int index) const { return sqlite3_column_double(stmt_, index); }

std::string Statement::column_text(int index) const {
  const auto* bytes = sqlite3_column_text(stmt_, index);
  if (bytes == nullptr) {
    return std::string{};
  }
  const int size = sqlite3_column_bytes(stmt_, index);
  return std::string{reinterpret_cast<const char*>(bytes), static_cast<std::size_t>(size)};
}

std::optional<std::int64_t> Statement::column_opt_int64(int index) const {
  if (is_null(index)) {
    return std::nullopt;
  }
  return column_int64(index);
}

std::optional<double> Statement::column_opt_double(int index) const {
  if (is_null(index)) {
    return std::nullopt;
  }
  return column_double(index);
}

std::optional<std::string> Statement::column_opt_text(int index) const {
  if (is_null(index)) {
    return std::nullopt;
  }
  return column_text(index);
}

}  // namespace coins::db
