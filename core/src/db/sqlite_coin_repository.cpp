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

// The `coin` columns, in the order `map_row` reads them. All names are unique to
// the coin table, so they are unambiguous even when joined against other tables.
constexpr std::string_view kCoinColumns =
    "id, country, denomination, face_value, coin_currency, year_from, year_to, "
    "mint, mint_mark, composition, weight_g, diameter_mm, grade_scale, grade_numeric, "
    "grade_label, acquired_date, acquired_price_eur, acquired_source, notes, created_at, "
    "updated_at";

// Binds the 18 caller-supplied ("mutable") columns starting at bind index 1, in
// the order used by both the INSERT and UPDATE statements. Returns the next
// free bind index so callers can append the timestamp/id parameters.
int bind_mutable_columns(Statement& stmt, const Coin& coin) {
  int index = 1;
  stmt.bind(index++, std::string_view{coin.country});
  stmt.bind(index++, coin.denomination);
  stmt.bind(index++, coin.face_value);
  stmt.bind(index++, coin.coin_currency);
  stmt.bind(index++, coin.year_from);
  stmt.bind(index++, coin.year_to);
  stmt.bind(index++, coin.mint);
  stmt.bind(index++, coin.mint_mark);
  stmt.bind(index++, coin.composition);
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

// Reads a full coin from the current row of a SELECT using `kSelectColumns`.
Coin map_row(Statement& stmt) {
  Coin coin;
  coin.id = stmt.column_int64(0);
  coin.country = stmt.column_text(1);
  coin.denomination = stmt.column_opt_text(2);
  coin.face_value = stmt.column_opt_double(3);
  coin.coin_currency = stmt.column_opt_text(4);
  coin.year_from = static_cast<int>(stmt.column_int64(5));
  coin.year_to = static_cast<int>(stmt.column_int64(6));
  coin.mint = stmt.column_opt_text(7);
  coin.mint_mark = stmt.column_opt_text(8);
  coin.composition = stmt.column_opt_text(9);
  coin.weight_g = stmt.column_opt_double(10);
  coin.diameter_mm = stmt.column_opt_double(11);
  coin.grade_scale = stmt.column_opt_text(12);
  if (const std::optional<std::int64_t> grade = stmt.column_opt_int64(13); grade.has_value()) {
    coin.grade_numeric = static_cast<int>(*grade);
  }
  coin.grade_label = stmt.column_opt_text(14);
  coin.acquired_date = stmt.column_opt_text(15);
  coin.acquired_price_eur = stmt.column_opt_double(16);
  coin.acquired_source = stmt.column_opt_text(17);
  coin.notes = stmt.column_opt_text(18);
  coin.created_at = stmt.column_text(19);
  coin.updated_at = stmt.column_text(20);
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
      "INSERT INTO coin (country, denomination, face_value, coin_currency, year_from, "
      "year_to, mint, mint_mark, composition, weight_g, diameter_mm, grade_scale, "
      "grade_numeric, grade_label, acquired_date, acquired_price_eur, acquired_source, "
      "notes, created_at, updated_at) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
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

  if (query.country.has_value()) {
    conditions.emplace_back("c.country = ? COLLATE NOCASE");
    params.emplace_back(*query.country);
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
    conditions.emplace_back("c.denomination LIKE ? COLLATE NOCASE");
    params.emplace_back("%" + *query.denomination + "%");
  }
  if (query.grade_label.has_value()) {
    conditions.emplace_back("c.grade_label = ? COLLATE NOCASE");
    params.emplace_back(*query.grade_label);
  }
  if (query.composition.has_value()) {
    conditions.emplace_back("c.composition LIKE ? COLLATE NOCASE");
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
    conditions.emplace_back(
        "(c.country LIKE ? COLLATE NOCASE OR c.denomination LIKE ? COLLATE NOCASE OR "
        "c.notes LIKE ? COLLATE NOCASE OR EXISTS (SELECT 1 FROM reference_link rl "
        "WHERE rl.coin_id = c.id AND rl.label LIKE ? COLLATE NOCASE))");
    const std::string like = "%" + *query.text + "%";
    params.emplace_back(like);
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
      sql += "c.country" + direction;
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
      "UPDATE coin SET country = ?, denomination = ?, face_value = ?, coin_currency = ?, "
      "year_from = ?, year_to = ?, mint = ?, mint_mark = ?, composition = ?, weight_g = ?, "
      "diameter_mm = ?, grade_scale = ?, grade_numeric = ?, grade_label = ?, "
      "acquired_date = ?, acquired_price_eur = ?, acquired_source = ?, notes = ?, "
      "updated_at = ? WHERE id = ?;");
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
