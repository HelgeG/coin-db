#ifndef COINS_DB_SCHEMA_HPP
#define COINS_DB_SCHEMA_HPP

#include <string_view>

#include "coins/db/database.hpp"

namespace coins::db {

/// Current schema version, recorded in SQLite's `PRAGMA user_version`. Bump this
/// when the schema changes and add a migration step in `bootstrap_schema`.
inline constexpr int kSchemaVersion = 1;

/// The DDL that defines the coins-db schema (tables + indexes). Idempotent:
/// every statement uses IF NOT EXISTS.
[[nodiscard]] std::string_view schema_sql() noexcept;

/// Creates the schema if it is not already present and records the schema
/// version. Safe to call on every startup; runs inside a single transaction.
void bootstrap_schema(Database& db);

/// Reads `PRAGMA user_version` for the connection.
[[nodiscard]] int read_schema_version(Database& db);

}  // namespace coins::db

#endif  // COINS_DB_SCHEMA_HPP
