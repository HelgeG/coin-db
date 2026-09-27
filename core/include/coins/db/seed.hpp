#ifndef COINS_DB_SEED_HPP
#define COINS_DB_SEED_HPP

#include "coins/db/database.hpp"

namespace coins::db {

/// Seeds the controlled-vocabulary tables with the built-in country and currency
/// data (and standard currency units). Idempotent: entries are keyed by
/// `(kind, code)` and units by `(currency_id, code)`, so re-running inserts
/// nothing new. Must run inside an open transaction (see `bootstrap_schema`).
void seed_lookups(Database& db);

}  // namespace coins::db

#endif  // COINS_DB_SEED_HPP
