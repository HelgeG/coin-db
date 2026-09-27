#ifndef COINS_DB_SCHEMA_HPP
#define COINS_DB_SCHEMA_HPP

#include <string_view>

#include "coins/db/database.hpp"

namespace coins::db {

/// Current schema version, recorded in SQLite's `PRAGMA user_version`. Bump this
/// when the schema changes and add a migration step in `bootstrap_schema`.
///
/// v2 introduced the localized controlled-vocabulary tables (`lookup_entry`,
/// `lookup_name`, `currency_unit`, `currency_unit_name`) and moved the coin's
/// encoded fields to `*_id` foreign keys.
///
/// v3 added the `app_setting` table (collection base currency), and renamed the
/// money columns `value_estimate.amount_eur` -> `amount` and
/// `coin.acquired_price_eur` -> `acquired_price` (amounts are now in the
/// user-selectable base currency). Because there is no production data, each
/// bump is a clean redefinition rather than a data-preserving migration.
inline constexpr int kSchemaVersion = 3;

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
