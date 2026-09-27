#include "coins/db/sqlite_coin_repository.hpp"

#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "coins/db/database.hpp"
#include "coins/db/statement.hpp"

namespace coins::db {
namespace {

// The `coin` columns, in the order `map_row` reads them.
constexpr std::string_view kCoinColumns =
    "id, country_id, denomination_id, face_value, currency_id, face_unit_id, year_from, "
    "year_to, mint_id, mint_mark, composition_id, weight_g, diameter_mm, grade_scale, "
    "grade_numeric, grade_label, acquired_date, acquired_price_eur, acquired_source, notes, "
    "created_at, updated_at";

// Binds the 19 caller-supplied ("mutable") columns starting at bind index 1, in
// the order used by both the INSERT and UPDATE statements. Returns the next
// free bind index so callers can append the timestamp/id parameters.
int bind_mutable_columns(Statement& stmt, const Coin& coin) {
  int index = 1;
  stmt.bind(index++, coin.country_id);
  stmt.bind(index++, coin.denomination_id);
  stmt.bind(index++, coin.face_value);
  stmt.bind(index++, coin.currency_id);
  stmt.bind(index++, coin.face_unit_id);
  stmt.bind(index++, coin.year_from);
  stmt.bind(index++, coin.year_to);
  stmt.bind(index++, coin.mint_id);
  stmt.bind(index++, coin.mint_mark);
  stmt.bind(index++, coin.composition_id);
  stmt.bind(index++, coin.weight_g);
  stmt.bind(index++, coin.diameter_mm);
  stmt.bind(index++, coin.grade_scale);
  stmt.bind(index++, coin.grade_numeric);
  stmt.bind(index++, coin.grade_label);
  stmt.bind(index++, coin.acquired_date);
  stmt.bind(index++, coin.acquired_price_eur);
  stmt.bind(index++, coin.acquired_source);
  stmt.bind(index++, coin.notes);
  return index;
}

// Reads a full coin from the current row of a SELECT using `kCoinColumns`.
Coin map_row(Statement& stmt) {
  Coin coin;
  coin.id = stmt.column_int64(0);
  coin.country_id = stmt.column_int64(1);
  if (const auto v = stmt.column_opt_int64(2)) coin.denomination_id = *v;
  coin.face_value = stmt.column_opt_double(3);
  if (const auto v = stmt.column_opt_int64(4)) coin.currency_id = *v;
  if (const auto v = stmt.column_opt_int64(5)) coin.face_unit_id = *v;
  coin.year_from = static_cast<int>(stmt.column_int64(6));
  coin.year_to = static_cast<int>(stmt.column_int64(7));
  if (const auto v = stmt.column_opt_int64(8)) coin.mint_id = *v;
  coin.mint_mark = stmt.column_opt_text(9);
  if (const auto v = stmt.column_opt_int64(10)) coin.composition_id = *v;
  coin.weight_g = stmt.column_opt_double(11);
  coin.diameter_mm = stmt.column_opt_double(12);
  coin.grade_scale = stmt.column_opt_text(13);
  if (const std::optional<std::int64_t> grade = stmt.column_opt_int64(14); grade.has_value()) {
    coin.grade_numeric = static_cast<int>(*grade);
  }
  coin.grade_label = stmt.column_opt_text(15);
  coin.acquired_date = stmt.column_opt_text(16);
  coin.acquired_price_eur = stmt.column_opt_double(17);
  coin.acquired_source = stmt.column_opt_text(18);
  coin.notes = stmt.column_opt_text(19);
  coin.created_at = stmt.column_text(20);
  coin.updated_at = stmt.column_text(21);
  return coin;
}

}  // namespace

SqliteCoinRepository::SqliteCoinRepository(Database& db, const IClock& clock)
    : db_(db), clock_(clock) {}

std::expected<Coin, ValidationErrors> SqliteCoinRepository::create(const Coin& input) {
  if (ValidationResult valid = validate_coin(input); !valid) {
    return std::unexpected(std::move(valid).error());
  }

  Coin coin = input;
  const std::string now = clock_.now_iso8601();
  coin.created_at = now;
  coin.updated_at = now;

  Statement stmt = db_.prepare(
      "INSERT INTO coin (country_id, denomination_id, face_value, currency_id, face_unit_id, "
      "year_from, year_to, mint_id, mint_mark, composition_id, weight_g, diameter_mm, "
      "grade_scale, grade_numeric, grade_label, acquired_date, acquired_price_eur, "
      "acquired_source, notes, created_at, updated_at) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
  int index = bind_mutable_columns(stmt, coin);
  stmt.bind(index++, std::string_view{coin.created_at});
  stmt.bind(index++, std::string_view{coin.updated_at});
  (void)stmt.step();

  coin.id = db_.last_insert_rowid();
  return coin;
}

std::optional<Coin> SqliteCoinRepository::get(Id id) {
  Statement stmt = db_.prepare("SELECT " + std::string{kCoinColumns} + " FROM coin WHERE id = ?;");
  stmt.bind(1, id);
  if (!stmt.step()) {
    return std::nullopt;
  }
  return map_row(stmt);
}

std::vector<Coin> SqliteCoinRepository::list() {
  std::vector<Coin> coins;
  Statement stmt = db_.prepare("SELECT " + std::string{kCoinColumns} + " FROM coin ORDER BY id;");
  while (stmt.step()) {
    coins.push_back(map_row(stmt));
  }
  return coins;
}

std::vector<Coin> SqliteCoinRepository::search(const CoinQuery& query) {
  using Param = std::variant<std::int64_t, double, std::string>;
  std::vector<std::string> conditions;
  std::vector<Param> params;

  // Matches a coin's lookup field (by column) against a name (substring, any
  // language, NOCASE) or an exact code, via an EXISTS over lookup_entry/name.
  auto lookup_match = [](std::string_view id_col) {
    return "EXISTS (SELECT 1 FROM lookup_entry e WHERE e.id = c." + std::string(id_col) +
           " AND (e.code = ? COLLATE NOCASE OR EXISTS (SELECT 1 FROM lookup_name n "
           "WHERE n.entry_id = e.id AND n.name LIKE ? COLLATE NOCASE)))";
  };

  if (query.country.has_value()) {
    conditions.emplace_back(lookup_match("country_id"));
    params.emplace_back(*query.country);
    params.emplace_back("%" + *query.country + "%");
  }
  if (query.year_from.has_value()) {
    // Range overlap: keep coins whose range ends at or after the lower bound.
    conditions.emplace_back("c.year_to >= ?");
    params.emplace_back(static_cast<std::int64_t>(*query.year_from));
  }
  if (query.year_to.has_value()) {
    conditions.emplace_back("c.year_from <= ?");
    params.emplace_back(static_cast<std::int64_t>(*query.year_to));
  }
  if (query.denomination.has_value()) {
    conditions.emplace_back(lookup_match("denomination_id"));
    params.emplace_back(*query.denomination);
    params.emplace_back("%" + *query.denomination + "%");
  }
  if (query.grade_label.has_value()) {
    conditions.emplace_back("c.grade_label = ? COLLATE NOCASE");
    params.emplace_back(*query.grade_label);
  }
  if (query.composition.has_value()) {
    conditions.emplace_back(lookup_match("composition_id"));
    params.emplace_back(*query.composition);
    params.emplace_back("%" + *query.composition + "%");
  }
  if (query.min_value_eur.has_value()) {
    conditions.emplace_back("le.amount_eur >= ?");
    params.emplace_back(*query.min_value_eur);
  }
  if (query.max_value_eur.has_value()) {
    conditions.emplace_back("le.amount_eur <= ?");
    params.emplace_back(*query.max_value_eur);
  }
  if (query.text.has_value()) {
    // Free text over the coin's localized lookup names (country/denomination),
    // notes, and reference-link labels.
    conditions.emplace_back(
        "(EXISTS (SELECT 1 FROM lookup_name n WHERE n.entry_id IN "
        "(c.country_id, c.denomination_id) AND n.name LIKE ? COLLATE NOCASE) "
        "OR c.notes LIKE ? COLLATE NOCASE OR EXISTS (SELECT 1 FROM reference_link rl "
        "WHERE rl.coin_id = c.id AND rl.label LIKE ? COLLATE NOCASE))");
    const std::string like = "%" + *query.text + "%";
    params.emplace_back(like);
    params.emplace_back(like);
    params.emplace_back(like);
  }

  // Join each coin to its latest estimate so value filters/sorting can use it.
  std::string sql = "SELECT " + std::string{kCoinColumns} +
                    " FROM coin c LEFT JOIN (SELECT coin_id, amount_eur, ROW_NUMBER() OVER "
                    "(PARTITION BY coin_id ORDER BY estimated_at DESC, id DESC) AS rn "
                    "FROM value_estimate) le ON le.coin_id = c.id AND le.rn = 1";
  for (std::size_t i = 0; i < conditions.size(); ++i) {
    sql += (i == 0) ? " WHERE " : " AND ";
    sql += conditions[i];
  }

  const std::string direction =
      query.sort_direction == SortDirection::Descending ? " DESC" : " ASC";
  sql += " ORDER BY ";
  switch (query.sort_field) {
    case SortField::Year:
      sql += "c.year_from" + direction;
      break;
    case SortField::Country:
      // Sort by the country entry's localized name in the active language,
      // falling back to the code. The lang param is appended below.
      sql +=
          "(SELECT COALESCE(MAX(CASE WHEN n.lang = ? THEN n.name END), e.code) "
          "FROM lookup_entry e LEFT JOIN lookup_name n ON n.entry_id = e.id "
          "WHERE e.id = c.country_id)" +
          direction;
      params.emplace_back(query.lang);
      break;
    case SortField::ValueEur:
      // NULLs (no estimate) always sort last, regardless of direction.
      sql += "le.amount_eur IS NULL, le.amount_eur" + direction;
      break;
    case SortField::DateAdded:
      sql += "c.created_at" + direction;
      break;
  }
  sql += ", c.id ASC;";  // stable tie-break

  Statement stmt = db_.prepare(sql);
  for (std::size_t i = 0; i < params.size(); ++i) {
    std::visit([&](const auto& value) { stmt.bind(static_cast<int>(i) + 1, value); }, params[i]);
  }

  std::vector<Coin> coins;
  while (stmt.step()) {
    coins.push_back(map_row(stmt));
  }
  return coins;
}

std::expected<bool, ValidationErrors> SqliteCoinRepository::update(const Coin& input) {
  if (ValidationResult valid = validate_coin(input); !valid) {
    return std::unexpected(std::move(valid).error());
  }

  const std::string now = clock_.now_iso8601();
  Statement stmt = db_.prepare(
      "UPDATE coin SET country_id = ?, denomination_id = ?, face_value = ?, currency_id = ?, "
      "face_unit_id = ?, year_from = ?, year_to = ?, mint_id = ?, mint_mark = ?, "
      "composition_id = ?, weight_g = ?, diameter_mm = ?, grade_scale = ?, grade_numeric = ?, "
      "grade_label = ?, acquired_date = ?, acquired_price_eur = ?, acquired_source = ?, "
      "notes = ?, updated_at = ? WHERE id = ?;");
  int index = bind_mutable_columns(stmt, input);
  stmt.bind(index++, std::string_view{now});  // updated_at (created_at is preserved)
  stmt.bind(index++, input.id);               // WHERE id
  (void)stmt.step();

  return db_.changes() > 0;
}

bool SqliteCoinRepository::remove(Id id) {
  Statement stmt = db_.prepare("DELETE FROM coin WHERE id = ?;");
  stmt.bind(1, id);
  (void)stmt.step();
  return db_.changes() > 0;
}

}  // namespace coins::db
