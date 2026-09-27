#include "coins/db/seed.hpp"

#include <array>
#include <string_view>
#include <utility>

#include "coins/db/statement.hpp"
#include "coins/id.hpp"

namespace coins::db {
namespace {

struct LangName {
  std::string_view en;
  std::string_view nb;
};

struct CountrySeed {
  std::string_view code;  // ISO 3166-1 alpha-2, or ISO 3166-3 for historical
  LangName name;
};

struct UnitSeed {
  std::string_view code;
  int minor_per_unit;
  bool is_major;
  LangName name;
};

struct CurrencySeed {
  std::string_view code;  // ISO 4217 (current or historical)
  LangName name;
  std::array<UnitSeed, 2> units;  // {major, minor}
};

// Representative set of current countries plus common historical states. Not
// the exhaustive ISO list — users add any missing country on the fly.
constexpr std::array<CountrySeed, 24> kCountries = {{
    {"NO", {"Norway", "Norge"}},
    {"SE", {"Sweden", "Sverige"}},
    {"DK", {"Denmark", "Danmark"}},
    {"FI", {"Finland", "Finland"}},
    {"IS", {"Iceland", "Island"}},
    {"US", {"United States", "USA"}},
    {"CA", {"Canada", "Canada"}},
    {"GB", {"United Kingdom", "Storbritannia"}},
    {"IE", {"Ireland", "Irland"}},
    {"FR", {"France", "Frankrike"}},
    {"DE", {"Germany", "Tyskland"}},
    {"NL", {"Netherlands", "Nederland"}},
    {"BE", {"Belgium", "Belgia"}},
    {"IT", {"Italy", "Italia"}},
    {"ES", {"Spain", "Spania"}},
    {"PT", {"Portugal", "Portugal"}},
    {"CH", {"Switzerland", "Sveits"}},
    {"AT", {"Austria", "Østerrike"}},
    {"PL", {"Poland", "Polen"}},
    {"RU", {"Russia", "Russland"}},
    // Historical states (ISO 3166-3 formerly-used codes).
    {"YUCS", {"Yugoslavia", "Jugoslavia"}},
    {"CSHH", {"Czechoslovakia", "Tsjekkoslovakia"}},
    {"SUHH", {"Soviet Union", "Sovjetunionen"}},
    {"DDDE", {"East Germany", "Øst-Tyskland"}},
}};

// Common currencies (current + historical) with their major/minor units.
constexpr std::array<CurrencySeed, 9> kCurrencies = {{
    {"NOK",
     {"Norwegian krone", "norsk krone"},
     {{{"krone", 100, true, {"krone", "krone"}}, {"ore", 1, false, {"øre", "øre"}}}}},
    {"SEK",
     {"Swedish krona", "svensk krone"},
     {{{"krona", 100, true, {"krona", "krona"}}, {"ore", 1, false, {"öre", "öre"}}}}},
    {"DKK",
     {"Danish krone", "dansk krone"},
     {{{"krone", 100, true, {"krone", "krone"}}, {"ore", 1, false, {"øre", "øre"}}}}},
    {"USD",
     {"US dollar", "amerikansk dollar"},
     {{{"dollar", 100, true, {"dollar", "dollar"}}, {"cent", 1, false, {"cent", "cent"}}}}},
    {"GBP",
     {"Pound sterling", "britisk pund"},
     {{{"pound", 100, true, {"pound", "pund"}}, {"penny", 1, false, {"penny", "penny"}}}}},
    {"EUR",
     {"Euro", "euro"},
     {{{"euro", 100, true, {"euro", "euro"}}, {"cent", 1, false, {"cent", "cent"}}}}},
    {"CHF",
     {"Swiss franc", "sveitsisk franc"},
     {{{"franc", 100, true, {"franc", "franc"}}, {"rappen", 1, false, {"rappen", "rappen"}}}}},
    // Historical currencies.
    {"DEM",
     {"Deutsche Mark", "tysk mark"},
     {{{"mark", 100, true, {"mark", "mark"}}, {"pfennig", 1, false, {"pfennig", "pfennig"}}}}},
    {"XXX",
     {"No currency", "ingen valuta"},
     {{{"unit", 1, true, {"unit", "enhet"}}, {"unit", 1, true, {"unit", "enhet"}}}}},
}};

/// Inserts a lookup entry if `(kind, code)` is absent; returns its id either way.
Id ensure_entry(Database& db, std::string_view kind, std::string_view code, const LangName& name) {
  {
    Statement find = db.prepare("SELECT id FROM lookup_entry WHERE kind = ? AND code = ?;");
    find.bind(1, kind);
    find.bind(2, code);
    if (find.step()) {
      return find.column_int64(0);
    }
  }
  Statement ins = db.prepare("INSERT INTO lookup_entry (kind, code) VALUES (?, ?);");
  ins.bind(1, kind);
  ins.bind(2, code);
  (void)ins.step();
  const Id entry_id = db.last_insert_rowid();

  for (const auto& [lang, value] :
       {std::pair{std::string_view{"en"}, name.en}, std::pair{std::string_view{"nb"}, name.nb}}) {
    Statement n = db.prepare("INSERT INTO lookup_name (entry_id, lang, name) VALUES (?, ?, ?);");
    n.bind(1, entry_id);
    n.bind(2, lang);
    n.bind(3, value);
    (void)n.step();
  }
  return entry_id;
}

void ensure_unit(Database& db, Id currency_id, const UnitSeed& unit) {
  {
    Statement find = db.prepare("SELECT id FROM currency_unit WHERE currency_id = ? AND code = ?;");
    find.bind(1, currency_id);
    find.bind(2, unit.code);
    if (find.step()) {
      return;
    }
  }
  Statement ins = db.prepare(
      "INSERT INTO currency_unit (currency_id, code, minor_per_unit, is_major) "
      "VALUES (?, ?, ?, ?);");
  ins.bind(1, currency_id);
  ins.bind(2, unit.code);
  ins.bind(3, unit.minor_per_unit);
  ins.bind(4, unit.is_major ? 1 : 0);
  (void)ins.step();
  const Id unit_id = db.last_insert_rowid();

  for (const auto& [lang, value] : {std::pair{std::string_view{"en"}, unit.name.en},
                                    std::pair{std::string_view{"nb"}, unit.name.nb}}) {
    Statement n =
        db.prepare("INSERT INTO currency_unit_name (unit_id, lang, name) VALUES (?, ?, ?);");
    n.bind(1, unit_id);
    n.bind(2, lang);
    n.bind(3, value);
    (void)n.step();
  }
}

}  // namespace

void seed_lookups(Database& db) {
  for (const CountrySeed& c : kCountries) {
    (void)ensure_entry(db, "country", c.code, c.name);
  }
  for (const CurrencySeed& c : kCurrencies) {
    const Id currency_id = ensure_entry(db, "currency", c.code, c.name);
    // units[0] is the major unit; units[1] the minor. Skip a duplicate minor
    // (used only as a placeholder for single-unit currencies like XXX).
    ensure_unit(db, currency_id, c.units[0]);
    if (c.units[1].code != c.units[0].code) {
      ensure_unit(db, currency_id, c.units[1]);
    }
  }
}

void seed_base_currency(Database& db) {
  // Already set? Leave it untouched (idempotent).
  {
    Statement find = db.prepare("SELECT value FROM app_setting WHERE key = 'base_currency_id';");
    if (find.step()) {
      return;
    }
  }
  // Default to the EUR currency entry.
  Statement eur =
      db.prepare("SELECT id FROM lookup_entry WHERE kind = 'currency' AND code = 'EUR';");
  if (!eur.step()) {
    return;  // No EUR seeded (shouldn't happen); leave unset.
  }
  const Id eur_id = eur.column_int64(0);
  Statement set =
      db.prepare("INSERT INTO app_setting (key, value) VALUES ('base_currency_id', ?);");
  set.bind(1, std::to_string(eur_id));
  (void)set.step();
}

}  // namespace coins::db
