#ifndef COINS_DB_SEED_HPP
#define COINS_DB_SEED_HPP

#include "coins/db/database.hpp"

namespace coins::db {

/// Seeds the controlled-vocabulary tables with the built-in country and currency
/// data (and standard currency units). Idempotent: entries are keyed by
/// `(kind, code)` and units by `(currency_id, code)`, so re-running inserts
/// nothing new. Must run inside an open transaction (see `bootstrap_schema`).
void seed_lookups(Database& db);

/// Sets the collection base currency (`app_setting.base_currency_id`) to the EUR
/// currency entry when it is not already set. Idempotent; assumes `seed_lookups`
/// has run so the EUR entry exists. Must run inside an open transaction.
void seed_base_currency(Database& db);

}  // namespace coins::db

#endif  // COINS_DB_SEED_HPP
