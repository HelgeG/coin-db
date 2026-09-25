#ifndef COINS_DB_STATEMENT_HPP
#define COINS_DB_STATEMENT_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

struct sqlite3_stmt;

namespace coins::db {

/// RAII wrapper around a prepared SQLite statement.
///
/// Enforces parameterized access: values are bound by index rather than
/// interpolated into SQL. Bind indices are 1-based (SQLite convention); column
/// indices are 0-based. Owns the underlying `sqlite3_stmt` and finalizes it on
/// destruction. Movable, non-copyable.
class Statement {
 public:
  /// Takes ownership of an already-prepared statement handle. Normally obtained
  /// via `Database::prepare` rather than constructed directly.
  explicit Statement(sqlite3_stmt* stmt) noexcept;

  Statement(const Statement&) = delete;
  Statement& operator=(const Statement&) = delete;
  Statement(Statement&& other) noexcept;
  Statement& operator=(Statement&& other) noexcept;
  ~Statement();

  // --- Parameter binding (1-based indices) --------------------------------
  void bind(int index, std::int64_t value);
  void bind(int index, int value);  // widened to int64
  void bind(int index, double value);
  void bind(int index, std::string_view value);
  void bind(int index, std::nullptr_t);

  /// Binds an optional: the contained value when present, SQL NULL otherwise.
  template <class T>
  void bind(int index, const std::optional<T>& value) {
    if (value.has_value()) {
      bind(index, *value);
    } else {
      bind(index, nullptr);
    }
  }

  // --- Execution ----------------------------------------------------------
  /// Advances to the next result row. Returns true if a row is available,
  /// false when the statement has finished. Throws on error.
  [[nodiscard]] bool step();

  /// Resets the statement so it can be re-executed; bindings are retained.
  void reset();

  // --- Column access (0-based indices) ------------------------------------
  [[nodiscard]] int column_count() const noexcept;
  [[nodiscard]] bool is_null(int index) const;
  [[nodiscard]] std::int64_t column_int64(int index) const;
  [[nodiscard]] double column_double(int index) const;
  [[nodiscard]] std::string column_text(int index) const;

  [[nodiscard]] std::optional<std::int64_t> column_opt_int64(int index) const;
  [[nodiscard]] std::optional<double> column_opt_double(int index) const;
  [[nodiscard]] std::optional<std::string> column_opt_text(int index) const;

  [[nodiscard]] sqlite3_stmt* handle() const noexcept { return stmt_; }

 private:
  sqlite3_stmt* stmt_ = nullptr;
};

}  // namespace coins::db

#endif  // COINS_DB_STATEMENT_HPP
