#include "coins/settings_service.hpp"

#include <gtest/gtest.h>

#include <optional>

#include "coins/db/database.hpp"
#include "coins/db/schema.hpp"
#include "coins/db/sqlite_lookup_repository.hpp"
#include "coins/id.hpp"
#include "coins/lookup_entry.hpp"
#include "coins/lookup_kind.hpp"

namespace {

using coins::Id;
using coins::LookupEntry;
using coins::LookupKind;
using coins::SettingsService;
using coins::db::Database;
using coins::db::SqliteLookupRepository;

// Fixture: a fresh bootstrapped in-memory DB (seeds the currency/country
// vocabularies and the default base currency) plus a settings service over it.
class SettingsServiceTest : public ::testing::Test {
 protected:
  SettingsServiceTest() : db_(Database::in_memory()), lookups_(db_) {
    coins::db::bootstrap_schema(db_);
  }

  Id currency_id(std::string_view code) {
    const auto entry = lookups_.find_by_code(LookupKind::Currency, code);
    EXPECT_TRUE(entry.has_value()) << "missing seeded currency: " << code;
    return entry->id;
  }

  Database db_;
  SqliteLookupRepository lookups_;
};

TEST_F(SettingsServiceTest, FreshDatabaseDefaultsToEur) {
  SettingsService settings{db_, lookups_};
  const std::optional<LookupEntry> base = settings.base_currency();
  ASSERT_TRUE(base.has_value());
  EXPECT_EQ(base->kind, LookupKind::Currency);
  EXPECT_EQ(base->code, "EUR");
}

TEST_F(SettingsServiceTest, SetBaseCurrencyToNokTakesEffect) {
  SettingsService settings{db_, lookups_};
  const Id nok = currency_id("NOK");

  EXPECT_TRUE(settings.set_base_currency(nok));

  const std::optional<LookupEntry> base = settings.base_currency();
  ASSERT_TRUE(base.has_value());
  EXPECT_EQ(base->id, nok);
  EXPECT_EQ(base->code, "NOK");
}

TEST_F(SettingsServiceTest, SetBaseCurrencyRejectsNonCurrencyAndLeavesUnchanged) {
  SettingsService settings{db_, lookups_};

  // A country entry is not a currency; setting it must fail.
  const auto norway = lookups_.find_by_code(LookupKind::Country, "NO");
  ASSERT_TRUE(norway.has_value());

  EXPECT_FALSE(settings.set_base_currency(norway->id));

  // The base currency is unchanged (still the EUR default).
  const std::optional<LookupEntry> base = settings.base_currency();
  ASSERT_TRUE(base.has_value());
  EXPECT_EQ(base->code, "EUR");
}

}  // namespace
