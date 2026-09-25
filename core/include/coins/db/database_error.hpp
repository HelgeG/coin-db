#ifndef COINS_DB_DATABASE_ERROR_HPP
#define COINS_DB_DATABASE_ERROR_HPP

#include <stdexcept>
#include <string>

namespace coins::db {

/// Thrown when a SQLite operation fails. This is reserved for *exceptional*
/// storage failures (I/O errors, constraint violations, API misuse) — not for
/// expected domain validation failures, which are reported via result types in
/// later phases (see design.md, "Error handling").
class DatabaseError : public std::runtime_error {
 public:
  DatabaseError(int code, const std::string& message) : std::runtime_error(message), code_(code) {}

  /// The underlying SQLite result code (e.g. SQLITE_CONSTRAINT).
  [[nodiscard]] int code() const noexcept { return code_; }

 private:
  int code_;
};

}  // namespace coins::db

#endif  // COINS_DB_DATABASE_ERROR_HPP
