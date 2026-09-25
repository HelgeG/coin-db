#include "coins/collection_io.hpp"

#include <cstdint>
#include <format>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "coins/coin.hpp"
#include "coins/db/database.hpp"
#include "coins/db/statement.hpp"
#include "coins/image.hpp"
#include "coins/json.hpp"
#include "coins/reference_link.hpp"
#include "coins/value_estimate.hpp"

namespace coins {
namespace {

using json = nlohmann::json;
using db::Statement;

constexpr std::string_view kCoinColumns =
    "id, country, denomination, face_value, coin_currency, year_from, year_to, "
    "mint, mint_mark, composition, weight_g, diameter_mm, grade_scale, grade_numeric, "
    "grade_label, acquired_date, acquired_price_eur, acquired_source, notes, created_at, "
    "updated_at";

// --- Row readers ----------------------------------------------------------

Coin map_coin(Statement& stmt) {
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

std::vector<Coin> read_coins(db::Database& db) {
  std::vector<Coin> coins;
  Statement stmt = db.prepare("SELECT " + std::string{kCoinColumns} + " FROM coin ORDER BY id;");
  while (stmt.step()) {
    coins.push_back(map_coin(stmt));
  }
  return coins;
}

std::vector<ValueEstimate> read_estimates(db::Database& db, Id coin_id) {
  std::vector<ValueEstimate> estimates;
  Statement stmt = db.prepare(
      "SELECT id, coin_id, amount_eur, estimated_at, source FROM value_estimate "
      "WHERE coin_id = ? ORDER BY id;");
  stmt.bind(1, coin_id);
  while (stmt.step()) {
    ValueEstimate estimate;
    estimate.id = stmt.column_int64(0);
    estimate.coin_id = stmt.column_int64(1);
    estimate.amount_eur = stmt.column_double(2);
    estimate.estimated_at = stmt.column_text(3);
    estimate.source = stmt.column_opt_text(4);
    estimates.push_back(std::move(estimate));
  }
  return estimates;
}

std::vector<ReferenceLink> read_links(db::Database& db, Id coin_id) {
  std::vector<ReferenceLink> links;
  Statement stmt = db.prepare(
      "SELECT id, coin_id, label, url FROM reference_link WHERE coin_id = ? ORDER BY id;");
  stmt.bind(1, coin_id);
  while (stmt.step()) {
    ReferenceLink link;
    link.id = stmt.column_int64(0);
    link.coin_id = stmt.column_int64(1);
    link.label = stmt.column_text(2);
    link.url = stmt.column_text(3);
    links.push_back(std::move(link));
  }
  return links;
}

std::vector<Image> read_images(db::Database& db, Id coin_id) {
  std::vector<Image> images;
  Statement stmt = db.prepare(
      "SELECT id, coin_id, kind, stored_path, original_name, caption FROM image "
      "WHERE coin_id = ? ORDER BY id;");
  stmt.bind(1, coin_id);
  while (stmt.step()) {
    Image image;
    image.id = stmt.column_int64(0);
    image.coin_id = stmt.column_int64(1);
    if (const std::optional<std::string> kind = stmt.column_opt_text(2); kind.has_value()) {
      image.kind = image_kind_from_string(*kind);
    }
    image.stored_path = stmt.column_text(3);
    image.original_name = stmt.column_opt_text(4);
    image.caption = stmt.column_opt_text(5);
    images.push_back(std::move(image));
  }
  return images;
}

// --- Insert helpers (ids preserved) ---------------------------------------

void insert_coin(db::Database& db, const Coin& coin) {
  Statement stmt = db.prepare(
      "INSERT INTO coin (id, country, denomination, face_value, coin_currency, year_from, "
      "year_to, mint, mint_mark, composition, weight_g, diameter_mm, grade_scale, "
      "grade_numeric, grade_label, acquired_date, acquired_price_eur, acquired_source, notes, "
      "created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
      "?, ?);");
  int i = 1;
  stmt.bind(i++, coin.id);
  stmt.bind(i++, std::string_view{coin.country});
  stmt.bind(i++, coin.denomination);
  stmt.bind(i++, coin.face_value);
  stmt.bind(i++, coin.coin_currency);
  stmt.bind(i++, coin.year_from);
  stmt.bind(i++, coin.year_to);
  stmt.bind(i++, coin.mint);
  stmt.bind(i++, coin.mint_mark);
  stmt.bind(i++, coin.composition);
  stmt.bind(i++, coin.weight_g);
  stmt.bind(i++, coin.diameter_mm);
  stmt.bind(i++, coin.grade_scale);
  stmt.bind(i++, coin.grade_numeric);
  stmt.bind(i++, coin.grade_label);
  stmt.bind(i++, coin.acquired_date);
  stmt.bind(i++, coin.acquired_price_eur);
  stmt.bind(i++, coin.acquired_source);
  stmt.bind(i++, coin.notes);
  stmt.bind(i++, std::string_view{coin.created_at});
  stmt.bind(i++, std::string_view{coin.updated_at});
  (void)stmt.step();
}

void insert_estimate(db::Database& db, const ValueEstimate& estimate) {
  Statement stmt = db.prepare(
      "INSERT INTO value_estimate (id, coin_id, amount_eur, estimated_at, source) "
      "VALUES (?, ?, ?, ?, ?);");
  stmt.bind(1, estimate.id);
  stmt.bind(2, estimate.coin_id);
  stmt.bind(3, estimate.amount_eur);
  stmt.bind(4, std::string_view{estimate.estimated_at});
  stmt.bind(5, estimate.source);
  (void)stmt.step();
}

void insert_link(db::Database& db, const ReferenceLink& link) {
  Statement stmt =
      db.prepare("INSERT INTO reference_link (id, coin_id, label, url) VALUES (?, ?, ?, ?);");
  stmt.bind(1, link.id);
  stmt.bind(2, link.coin_id);
  stmt.bind(3, std::string_view{link.label});
  stmt.bind(4, std::string_view{link.url});
  (void)stmt.step();
}

void insert_image(db::Database& db, const Image& image) {
  Statement stmt = db.prepare(
      "INSERT INTO image (id, coin_id, kind, stored_path, original_name, caption) "
      "VALUES (?, ?, ?, ?, ?, ?);");
  stmt.bind(1, image.id);
  stmt.bind(2, image.coin_id);
  const std::optional<std::string> kind =
      image.kind.has_value() ? std::optional<std::string>{std::string{to_string(*image.kind)}}
                             : std::nullopt;
  stmt.bind(3, kind);
  stmt.bind(4, std::string_view{image.stored_path});
  stmt.bind(5, image.original_name);
  stmt.bind(6, image.caption);
  (void)stmt.step();
}

void append_prefixed(ValidationErrors& out, const ValidationErrors& in, const std::string& prefix) {
  for (const ValidationError& error : in) {
    out.push_back({prefix + error.field, error.message});
  }
}

// --- CSV ------------------------------------------------------------------

std::string csv_escape(const std::string& field) {
  const bool needs_quoting = field.find_first_of(",\"\n\r") != std::string::npos;
  if (!needs_quoting) {
    return field;
  }
  std::string escaped = "\"";
  for (const char ch : field) {
    if (ch == '"') {
      escaped += '"';  // double up embedded quotes
    }
    escaped += ch;
  }
  escaped += '"';
  return escaped;
}

std::string csv_opt_string(const std::optional<std::string>& value) {
  return value.has_value() ? csv_escape(*value) : std::string{};
}
std::string csv_opt_double(const std::optional<double>& value) {
  return value.has_value() ? std::format("{}", *value) : std::string{};
}
std::string csv_opt_int(const std::optional<int>& value) {
  return value.has_value() ? std::to_string(*value) : std::string{};
}

}  // namespace

std::string export_json(db::Database& db) {
  json root;
  root["version"] = 1;
  root["coins"] = json::array();
  for (const Coin& coin : read_coins(db)) {
    json coin_json = coin_to_json(coin);

    json estimates = json::array();
    for (const ValueEstimate& estimate : read_estimates(db, coin.id)) {
      estimates.push_back(value_estimate_to_json(estimate));
    }
    coin_json["value_estimates"] = std::move(estimates);

    json links = json::array();
    for (const ReferenceLink& link : read_links(db, coin.id)) {
      links.push_back(reference_link_to_json(link));
    }
    coin_json["reference_links"] = std::move(links);

    json images = json::array();
    for (const Image& image : read_images(db, coin.id)) {
      images.push_back(image_to_json(image));
    }
    coin_json["images"] = std::move(images);

    root["coins"].push_back(std::move(coin_json));
  }
  return root.dump(2);
}

std::expected<ImportStats, ValidationErrors> import_json(db::Database& db,
                                                         std::string_view json_text) {
  json root;
  try {
    root = json::parse(std::string{json_text});
  } catch (const json::exception& e) {
    return std::unexpected(ValidationErrors{{"json", std::string{"parse error: "} + e.what()}});
  }

  if (!root.is_object() || !root.contains("coins") || !root.at("coins").is_array()) {
    return std::unexpected(
        ValidationErrors{{"json", R"(expected an object with a "coins" array)"}});
  }

  struct CoinGraph {
    Coin coin;
    std::vector<ValueEstimate> estimates;
    std::vector<ReferenceLink> links;
    std::vector<Image> images;
  };

  std::vector<CoinGraph> graphs;
  ValidationErrors errors;
  const json& coins_json = root.at("coins");

  for (std::size_t i = 0; i < coins_json.size(); ++i) {
    const std::string prefix = "coins[" + std::to_string(i) + "].";
    const json& coin_json = coins_json.at(i);
    if (!coin_json.is_object()) {
      errors.push_back({"coins[" + std::to_string(i) + "]", "must be an object"});
      continue;
    }

    CoinGraph graph;
    graph.coin = coin_from_json(coin_json);
    if (ValidationResult valid = validate_coin(graph.coin); !valid) {
      append_prefixed(errors, valid.error(), prefix);
    }

    if (coin_json.contains("value_estimates") && coin_json.at("value_estimates").is_array()) {
      for (const json& estimate_json : coin_json.at("value_estimates")) {
        ValueEstimate estimate = value_estimate_from_json(estimate_json);
        estimate.coin_id = graph.coin.id;
        if (ValidationResult valid = validate_value_estimate(estimate); !valid) {
          append_prefixed(errors, valid.error(), prefix + "value_estimate.");
        }
        graph.estimates.push_back(std::move(estimate));
      }
    }

    if (coin_json.contains("reference_links") && coin_json.at("reference_links").is_array()) {
      for (const json& link_json : coin_json.at("reference_links")) {
        ReferenceLink link = reference_link_from_json(link_json);
        link.coin_id = graph.coin.id;
        if (ValidationResult valid = validate_reference_link(link); !valid) {
          append_prefixed(errors, valid.error(), prefix + "reference_link.");
        }
        graph.links.push_back(std::move(link));
      }
    }

    if (coin_json.contains("images") && coin_json.at("images").is_array()) {
      for (const json& image_json : coin_json.at("images")) {
        Image image = image_from_json(image_json);
        image.coin_id = graph.coin.id;
        if (image_json.contains("kind") && !image_json.at("kind").is_null() &&
            !image.kind.has_value()) {
          errors.push_back({prefix + "image.kind",
                            "unknown image kind: " + image_json.at("kind").get<std::string>()});
        }
        if (image.stored_path.empty()) {
          errors.push_back({prefix + "image.stored_path", "is required"});
        }
        graph.images.push_back(std::move(image));
      }
    }

    graphs.push_back(std::move(graph));
  }

  if (!errors.empty()) {
    return std::unexpected(std::move(errors));
  }

  // All valid: insert atomically with ids preserved.
  db.execute("BEGIN;");
  try {
    ImportStats stats;
    for (const CoinGraph& graph : graphs) {
      insert_coin(db, graph.coin);
      ++stats.coins;
      for (const ValueEstimate& estimate : graph.estimates) {
        insert_estimate(db, estimate);
        ++stats.value_estimates;
      }
      for (const ReferenceLink& link : graph.links) {
        insert_link(db, link);
        ++stats.reference_links;
      }
      for (const Image& image : graph.images) {
        insert_image(db, image);
        ++stats.images;
      }
    }
    db.execute("COMMIT;");
    return stats;
  } catch (...) {
    db.execute("ROLLBACK;");
    throw;
  }
}

std::string export_csv(db::Database& db) {
  std::string out =
      "id,country,denomination,face_value,coin_currency,year_from,year_to,mint,mint_mark,"
      "composition,weight_g,diameter_mm,grade_scale,grade_numeric,grade_label,acquired_date,"
      "acquired_price_eur,acquired_source,notes,created_at,updated_at,latest_estimate_eur,"
      "latest_estimate_at\n";

  Statement stmt = db.prepare(
      "SELECT " + std::string{kCoinColumns} +
      ", le.amount_eur, le.estimated_at FROM coin c LEFT JOIN (SELECT coin_id, amount_eur, "
      "estimated_at, ROW_NUMBER() OVER (PARTITION BY coin_id ORDER BY estimated_at DESC, id DESC) "
      "AS rn FROM value_estimate) le ON le.coin_id = c.id AND le.rn = 1 ORDER BY c.id;");

  while (stmt.step()) {
    const Coin coin = map_coin(stmt);
    const std::optional<double> latest_amount = stmt.column_opt_double(21);
    const std::optional<std::string> latest_at = stmt.column_opt_text(22);

    const std::string fields[] = {std::to_string(coin.id),
                                  csv_escape(coin.country),
                                  csv_opt_string(coin.denomination),
                                  csv_opt_double(coin.face_value),
                                  csv_opt_string(coin.coin_currency),
                                  std::to_string(coin.year_from),
                                  std::to_string(coin.year_to),
                                  csv_opt_string(coin.mint),
                                  csv_opt_string(coin.mint_mark),
                                  csv_opt_string(coin.composition),
                                  csv_opt_double(coin.weight_g),
                                  csv_opt_double(coin.diameter_mm),
                                  csv_opt_string(coin.grade_scale),
                                  csv_opt_int(coin.grade_numeric),
                                  csv_opt_string(coin.grade_label),
                                  csv_opt_string(coin.acquired_date),
                                  csv_opt_double(coin.acquired_price_eur),
                                  csv_opt_string(coin.acquired_source),
                                  csv_opt_string(coin.notes),
                                  csv_escape(coin.created_at),
                                  csv_escape(coin.updated_at),
                                  csv_opt_double(latest_amount),
                                  csv_opt_string(latest_at)};

    bool first = true;
    for (const std::string& field : fields) {
      if (!first) {
        out += ',';
      }
      out += field;
      first = false;
    }
    out += '\n';
  }
  return out;
}

}  // namespace coins
