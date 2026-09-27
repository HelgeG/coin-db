#include "server_app.hpp"

#include <httplib.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <optional>
#include <random>
#include <string>
#include <system_error>

#include "coins/coin.hpp"
#include "coins/coin_query.hpp"
#include "coins/collection_service.hpp"
#include "coins/collection_summary.hpp"
#include "coins/currency_unit.hpp"
#include "coins/id.hpp"
#include "coins/image.hpp"
#include "coins/json.hpp"
#include "coins/lookup_entry.hpp"
#include "coins/lookup_kind.hpp"
#include "coins/reference_link.hpp"
#include "coins/validation.hpp"
#include "coins/value_estimate.hpp"

namespace coins::server {
namespace {

using json = nlohmann::json;
using httplib::Request;
using httplib::Response;

// The active language for localized names. Read from a ?lang= query param,
// defaulting to English (the fallback language).
std::string lang_of(const Request& req) {
  if (req.has_param("lang")) {
    const std::string value = req.get_param_value("lang");
    if (!value.empty()) return value;
  }
  return "en";
}

void respond(Response& res, int status, const json& body) {
  res.status = status;
  res.set_content(body.dump(), "application/json");
}

void respond_error(Response& res, int status, std::string message) {
  respond(res, status, json{{"error", std::move(message)}});
}

void respond_validation(Response& res, const ValidationErrors& errors) {
  json items = json::array();
  for (const ValidationError& e : errors) {
    items.push_back(json{{"field", e.field}, {"message", e.message}});
  }
  respond(res, 422, json{{"errors", items}});
}

std::optional<json> parse_body(const Request& req, Response& res) {
  try {
    return json::parse(req.body);
  } catch (const json::exception&) {
    respond_error(res, 400, "invalid JSON body");
    return std::nullopt;
  }
}

std::optional<int> to_int(const std::string& text) {
  try {
    std::size_t pos = 0;
    const int value = std::stoi(text, &pos);
    return pos == text.size() ? std::optional<int>{value} : std::nullopt;
  } catch (...) {
    return std::nullopt;
  }
}

std::optional<double> to_double(const std::string& text) {
  try {
    std::size_t pos = 0;
    const double value = std::stod(text, &pos);
    return pos == text.size() ? std::optional<double>{value} : std::nullopt;
  } catch (...) {
    return std::nullopt;
  }
}

SortField sort_field_from(const std::string& name) {
  if (name == "year") return SortField::Year;
  if (name == "country") return SortField::Country;
  if (name == "value") return SortField::ValueEur;
  return SortField::DateAdded;
}

CoinQuery query_from_request(const Request& req) {
  CoinQuery query;
  query.lang = lang_of(req);
  if (req.has_param("country")) query.country = req.get_param_value("country");
  if (req.has_param("year_from")) query.year_from = to_int(req.get_param_value("year_from"));
  if (req.has_param("year_to")) query.year_to = to_int(req.get_param_value("year_to"));
  if (req.has_param("denomination")) query.denomination = req.get_param_value("denomination");
  if (req.has_param("grade")) query.grade_label = req.get_param_value("grade");
  if (req.has_param("metal")) query.composition = req.get_param_value("metal");
  if (req.has_param("min_eur")) query.min_value_eur = to_double(req.get_param_value("min_eur"));
  if (req.has_param("max_eur")) query.max_value_eur = to_double(req.get_param_value("max_eur"));
  if (req.has_param("text")) query.text = req.get_param_value("text");
  if (req.has_param("sort")) query.sort_field = sort_field_from(req.get_param_value("sort"));
  if (req.has_param("desc")) query.sort_direction = SortDirection::Descending;
  return query;
}

Id path_id(const Request& req) { return std::stoll(req.matches[1].str()); }

std::string random_hex() {
  static thread_local std::mt19937_64 rng{std::random_device{}()};
  std::uniform_int_distribution<std::uint64_t> dist;
  return std::format("{:016x}{:016x}", dist(rng), dist(rng));
}

std::string content_type_for(const std::filesystem::path& path) {
  std::string ext = path.extension().string();
  std::ranges::transform(ext, ext.begin(),
                         [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  if (ext == ".png") return "image/png";
  if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
  if (ext == ".gif") return "image/gif";
  if (ext == ".webp") return "image/webp";
  return "application/octet-stream";
}

std::optional<std::string> read_file_bytes(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return std::nullopt;
  }
  return std::string{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

json summary_to_json(const CollectionSummary& summary) {
  json buckets = json::array();
  for (const SummaryBucket& bucket : summary.breakdown.buckets) {
    buckets.push_back(json{{"label", bucket.label}, {"coin_count", bucket.coin_count}});
  }
  return json{{"coin_count", summary.coin_count},
              {"total_estimate_eur", summary.total_estimate_eur},
              {"breakdown",
               json{{"type", coins::to_string(summary.breakdown.type)}, {"buckets", buckets}}}};
}

// --- Localized, nested coin JSON ------------------------------------------
//
// The core `coin_to_json`/`coin_from_json` are id-based (used by import/export).
// The REST server presents a *localized, nested* shape instead: each encoded
// field serializes as {"id","code","name"} (localized) or null, and the face
// value serializes as face_value + face_unit. These helpers live here so the
// core json layer stays purely id-based.

// Serializes a lookup entry id as {"id","code","name"} (localized), or null.
json lookup_ref_json(CollectionService& service, std::optional<Id> id, std::string_view lang) {
  if (!id) return nullptr;
  const auto entry = service.lookup(*id);
  if (!entry) return nullptr;
  return json{{"id", entry->id}, {"code", entry->code}, {"name", entry->display_name(lang)}};
}

// Serializes a required lookup entry id (country), falling back to just the id
// if the entry is somehow missing.
json required_lookup_ref_json(CollectionService& service, Id id, std::string_view lang) {
  const auto entry = service.lookup(id);
  if (!entry) return json{{"id", id}, {"code", nullptr}, {"name", nullptr}};
  return json{{"id", entry->id}, {"code", entry->code}, {"name", entry->display_name(lang)}};
}

// Serializes a currency unit id as {"id","code","name"} (localized), or null
// (meaning the currency's major unit).
json unit_ref_json(CollectionService& service, std::optional<Id> id, std::string_view lang) {
  if (!id) return nullptr;
  const auto unit = service.currency_unit(*id);
  if (!unit) return nullptr;
  return json{{"id", unit->id}, {"code", unit->code}, {"name", unit->display_name(lang)}};
}

template <typename T>
json opt_json(const std::optional<T>& value) {
  if (!value) return nullptr;
  return json(*value);
}

// The localized coin response body (without child relations).
json coin_response_json(CollectionService& service, const Coin& coin, std::string_view lang) {
  return json{
      {"id", coin.id},
      {"country", required_lookup_ref_json(service, coin.country_id, lang)},
      {"denomination", lookup_ref_json(service, coin.denomination_id, lang)},
      {"currency", lookup_ref_json(service, coin.currency_id, lang)},
      {"mint", lookup_ref_json(service, coin.mint_id, lang)},
      {"composition", lookup_ref_json(service, coin.composition_id, lang)},
      {"face_value", opt_json(coin.face_value)},
      {"face_unit", unit_ref_json(service, coin.face_unit_id, lang)},
      {"mint_mark", opt_json(coin.mint_mark)},
      {"year_from", coin.year_from},
      {"year_to", coin.year_to},
      {"weight_g", opt_json(coin.weight_g)},
      {"diameter_mm", opt_json(coin.diameter_mm)},
      {"grade_scale", opt_json(coin.grade_scale)},
      {"grade_numeric", opt_json(coin.grade_numeric)},
      {"grade_label", opt_json(coin.grade_label)},
      {"acquired_date", opt_json(coin.acquired_date)},
      {"acquired_price", opt_json(coin.acquired_price)},
      {"acquired_source", opt_json(coin.acquired_source)},
      {"notes", opt_json(coin.notes)},
      {"created_at", coin.created_at},
      {"updated_at", coin.updated_at},
  };
}

// Extracts the free-text handle for an encoded field from the request JSON.
// Accepts a number (id), a string (code/name), or an object {"code":..} /
// {"name":..}. Returns the text to resolve, or nullopt for a numeric id (handled
// by the caller), or an absent/null field.
std::optional<std::string> encoded_text(const json& value, std::optional<Id>& out_id) {
  if (value.is_null()) return std::nullopt;
  if (value.is_number_integer()) {
    out_id = value.get<Id>();
    return std::nullopt;
  }
  if (value.is_string()) {
    return value.get<std::string>();
  }
  if (value.is_object()) {
    if (value.contains("id") && value.at("id").is_number_integer()) {
      out_id = value.at("id").get<Id>();
      return std::nullopt;
    }
    if (value.contains("code") && value.at("code").is_string()) {
      return value.at("code").get<std::string>();
    }
    if (value.contains("name") && value.at("name").is_string()) {
      return value.at("name").get<std::string>();
    }
  }
  return std::nullopt;
}

// Resolves an encoded field from the body into a lookup entry id. Sets `out`
// (or leaves it untouched when the field is absent/null). A numeric id is used
// directly; a string/object is resolved-or-created via the service.
void resolve_field(CollectionService& service, const json& body, const char* key, LookupKind kind,
                   std::string_view lang, std::optional<Id>& out) {
  if (!body.contains(key)) return;
  const json& value = body.at(key);
  std::optional<Id> id;
  const std::optional<std::string> text = encoded_text(value, id);
  if (id) {
    out = *id;
    return;
  }
  if (text && !text->empty()) {
    out = service.resolve_lookup(kind, lang, *text).id;
    return;
  }
  if (value.is_null()) out = std::nullopt;
}

template <typename T>
std::optional<T> read_opt(const json& body, const char* key) {
  if (!body.contains(key) || body.at(key).is_null()) return std::nullopt;
  return body.at(key).get<T>();
}

// Parses a coin from a create/update request body, resolving encoded fields
// (and the face unit) to ids via the service. `country` is required.
Coin coin_from_request(CollectionService& service, const json& body, std::string_view lang) {
  Coin coin;

  std::optional<Id> country_id;
  resolve_field(service, body, "country", LookupKind::Country, lang, country_id);
  coin.country_id = country_id.value_or(kUnsavedId);

  resolve_field(service, body, "denomination", LookupKind::Denomination, lang,
                coin.denomination_id);
  resolve_field(service, body, "currency", LookupKind::Currency, lang, coin.currency_id);
  resolve_field(service, body, "mint", LookupKind::Mint, lang, coin.mint_id);
  resolve_field(service, body, "composition", LookupKind::Composition, lang, coin.composition_id);

  coin.face_value = read_opt<double>(body, "face_value");

  // face_unit is resolved within the coin's currency (major unit when absent).
  if (coin.currency_id && body.contains("face_unit") && !body.at("face_unit").is_null()) {
    const json& value = body.at("face_unit");
    std::optional<Id> unit_id;
    const std::optional<std::string> text = encoded_text(value, unit_id);
    if (unit_id) {
      coin.face_unit_id = *unit_id;
    } else if (text && !text->empty()) {
      coin.face_unit_id = service.resolve_currency_unit(*coin.currency_id, lang, *text).id;
    }
  }

  coin.mint_mark = read_opt<std::string>(body, "mint_mark");
  coin.year_from = body.value("year_from", 0);
  coin.year_to = body.value("year_to", 0);
  coin.weight_g = read_opt<double>(body, "weight_g");
  coin.diameter_mm = read_opt<double>(body, "diameter_mm");
  coin.grade_scale = read_opt<std::string>(body, "grade_scale");
  coin.grade_numeric = read_opt<int>(body, "grade_numeric");
  coin.grade_label = read_opt<std::string>(body, "grade_label");
  coin.acquired_date = read_opt<std::string>(body, "acquired_date");
  coin.acquired_price = read_opt<double>(body, "acquired_price");
  coin.acquired_source = read_opt<std::string>(body, "acquired_source");
  coin.notes = read_opt<std::string>(body, "notes");
  return coin;
}

// {"id","code","name"} for a lookup entry, localized.
json lookup_entry_json(const LookupEntry& entry, std::string_view lang) {
  return json{{"id", entry.id}, {"code", entry.code}, {"name", entry.display_name(lang)}};
}

// {"id","code","name"} for a currency unit, localized.
json currency_unit_json(const CurrencyUnit& unit, std::string_view lang) {
  return json{{"id", unit.id}, {"code", unit.code}, {"name", unit.display_name(lang)}};
}

// Collection settings body: the base currency as {"id","code","name"} (localized),
// or null when unset.
json settings_json(CollectionService& service, std::string_view lang) {
  const auto base = service.base_currency();
  json base_json = nullptr;
  if (base) {
    base_json = json{{"id", base->id}, {"code", base->code}, {"name", base->display_name(lang)}};
  }
  return json{{"base_currency", base_json}};
}

}  // namespace

void register_routes(httplib::Server& server, CollectionService& service) {
  // Any unexpected throw (e.g. a storage error) becomes a 500 rather than
  // dropping the connection.
  server.set_exception_handler([](const Request&, Response& res, std::exception_ptr ep) {
    std::string message = "internal error";
    try {
      std::rethrow_exception(ep);
    } catch (const std::exception& e) {
      message = e.what();
    } catch (...) {
    }
    respond_error(res, 500, std::move(message));
  });

  // List / search.
  server.Get("/coins", [&service](const Request& req, Response& res) {
    const CoinQuery query = query_from_request(req);
    json coins = json::array();
    for (const Coin& coin : service.search(query)) {
      coins.push_back(coin_response_json(service, coin, query.lang));
    }
    respond(res, 200, coins);
  });

  // Create.
  server.Post("/coins", [&service](const Request& req, Response& res) {
    const auto body = parse_body(req, res);
    if (!body) return;
    const std::string lang = lang_of(req);
    const auto created = service.add_coin(coin_from_request(service, *body, lang));
    if (!created) {
      respond_validation(res, created.error());
      return;
    }
    respond(res, 201, coin_response_json(service, *created, lang));
  });

  // Get one, with relations.
  server.Get(R"(/coins/(\d+))", [&service](const Request& req, Response& res) {
    const Id id = path_id(req);
    const std::string lang = lang_of(req);
    const auto coin = service.get_coin(id);
    if (!coin) {
      respond_error(res, 404, "coin not found");
      return;
    }
    json body = coin_response_json(service, *coin, lang);
    json estimates = json::array();
    for (const ValueEstimate& e : service.estimate_history(id)) {
      estimates.push_back(value_estimate_to_json(e));
    }
    body["value_estimates"] = std::move(estimates);
    json links = json::array();
    for (const ReferenceLink& l : service.links(id)) {
      links.push_back(reference_link_to_json(l));
    }
    body["reference_links"] = std::move(links);
    json images = json::array();
    for (const Image& im : service.images(id)) {
      images.push_back(image_to_json(im));
    }
    body["images"] = std::move(images);
    respond(res, 200, body);
  });

  // Update.
  server.Put(R"(/coins/(\d+))", [&service](const Request& req, Response& res) {
    const Id id = path_id(req);
    const auto body = parse_body(req, res);
    if (!body) return;
    const std::string lang = lang_of(req);
    Coin coin = coin_from_request(service, *body, lang);
    coin.id = id;
    const auto updated = service.update_coin(coin);
    if (!updated) {
      respond_validation(res, updated.error());
      return;
    }
    if (!*updated) {
      respond_error(res, 404, "coin not found");
      return;
    }
    respond(res, 200, coin_response_json(service, *service.get_coin(id), lang));
  });

  // Delete.
  server.Delete(R"(/coins/(\d+))", [&service](const Request& req, Response& res) {
    const Id id = path_id(req);
    if (service.delete_coin(id)) {
      respond(res, 200, json{{"deleted", id}});
    } else {
      respond_error(res, 404, "coin not found");
    }
  });

  // Add estimate.
  server.Post(R"(/coins/(\d+)/estimates)", [&service](const Request& req, Response& res) {
    const Id coin_id = path_id(req);
    if (!service.get_coin(coin_id)) {
      respond_error(res, 404, "coin not found");
      return;
    }
    const auto body = parse_body(req, res);
    if (!body) return;
    ValueEstimate estimate = value_estimate_from_json(*body);
    estimate.coin_id = coin_id;
    if (estimate.estimated_at.empty()) {
      estimate.estimated_at = service.today();
    }
    const auto added = service.add_estimate(estimate);
    if (!added) {
      respond_validation(res, added.error());
      return;
    }
    respond(res, 201, value_estimate_to_json(*added));
  });

  // Add reference link.
  server.Post(R"(/coins/(\d+)/links)", [&service](const Request& req, Response& res) {
    const Id coin_id = path_id(req);
    if (!service.get_coin(coin_id)) {
      respond_error(res, 404, "coin not found");
      return;
    }
    const auto body = parse_body(req, res);
    if (!body) return;
    ReferenceLink link = reference_link_from_json(*body);
    link.coin_id = coin_id;
    const auto added = service.add_link(link);
    if (!added) {
      respond_validation(res, added.error());
      return;
    }
    respond(res, 201, reference_link_to_json(*added));
  });

  // Remove reference link.
  server.Delete(R"(/links/(\d+))", [&service](const Request& req, Response& res) {
    const Id id = path_id(req);
    if (service.remove_link(id)) {
      respond(res, 200, json{{"deleted", id}});
    } else {
      respond_error(res, 404, "link not found");
    }
  });

  // Upload image (multipart form-data: file, optional kind + caption).
  server.Post(R"(/coins/(\d+)/images)", [&service](const Request& req, Response& res) {
    const Id coin_id = path_id(req);
    if (!service.get_coin(coin_id)) {
      respond_error(res, 404, "coin not found");
      return;
    }
    if (!req.form.has_file("file")) {
      respond_error(res, 400, "missing 'file' part");
      return;
    }
    const httplib::FormData file = req.form.get_file("file");
    std::optional<ImageKind> kind;
    if (req.form.has_field("kind")) {
      kind = image_kind_from_string(req.form.get_field("kind"));
    }
    std::optional<std::string> caption;
    if (req.form.has_field("caption")) {
      caption = req.form.get_field("caption");
    }

    // Persist the uploaded bytes to a temp file (keeping the extension so the
    // store's type check works), hand it to the service, then clean up.
    const std::filesystem::path extension = std::filesystem::path{file.filename}.extension();
    const std::filesystem::path temp =
        std::filesystem::temp_directory_path() / (random_hex() + extension.string());
    {
      std::ofstream out(temp, std::ios::binary);
      out.write(file.content.data(), static_cast<std::streamsize>(file.content.size()));
    }
    std::optional<std::string> original_name;
    if (!file.filename.empty()) {
      original_name = file.filename;
    }
    const auto added =
        service.add_image(coin_id, temp, kind, std::move(caption), std::move(original_name));
    std::error_code ec;
    std::filesystem::remove(temp, ec);

    if (!added) {
      respond_validation(res, added.error());
      return;
    }
    respond(res, 201, image_to_json(*added));
  });

  // Serve an image's bytes. Looked up by id so no client-supplied path reaches
  // the filesystem (the stored path is resolved from our own DB row).
  server.Get(R"(/images/(\d+)/file)", [&service](const Request& req, Response& res) {
    const Id id = path_id(req);
    const auto image = service.get_image(id);
    if (!image) {
      respond_error(res, 404, "image not found");
      return;
    }
    const std::filesystem::path path = service.resolve_image(image->stored_path);
    const auto bytes = read_file_bytes(path);
    if (!bytes) {
      respond_error(res, 404, "image file missing");
      return;
    }
    res.status = 200;
    res.set_content(*bytes, content_type_for(path));
  });

  // Remove image.
  server.Delete(R"(/images/(\d+))", [&service](const Request& req, Response& res) {
    const Id id = path_id(req);
    if (service.remove_image(id)) {
      respond(res, 200, json{{"deleted", id}});
    } else {
      respond_error(res, 404, "image not found");
    }
  });

  // Collection summary.
  server.Get("/summary", [&service](const Request& req, Response& res) {
    // Unknown or missing ?type= falls back to the default summary. (?lang= is
    // accepted for parity with the other endpoints; localized bucket labels are
    // produced by the core summary layer.)
    coins::SummaryType type = coins::kDefaultSummaryType;
    if (req.has_param("type")) {
      if (const auto parsed = coins::summary_type_from_string(req.get_param_value("type"))) {
        type = *parsed;
      }
    }
    respond(res, 200, summary_to_json(service.summary(type)));
  });

  // Read collection settings (the base currency, localized).
  server.Get("/settings", [&service](const Request& req, Response& res) {
    respond(res, 200, settings_json(service, lang_of(req)));
  });

  // Update collection settings. Accepts { "base_currency": <id|code|name> } where
  // the value may be a number (id), a string (code/name), or an object with an
  // "id"/"code"/"name" field. Strings/objects are resolved against the currency
  // vocabulary; the resolved entry must be a currency.
  server.Put("/settings", [&service](const Request& req, Response& res) {
    const auto body = parse_body(req, res);
    if (!body) return;
    const std::string lang = lang_of(req);
    if (!body->contains("base_currency") || body->at("base_currency").is_null()) {
      respond_error(res, 400, "expected a 'base_currency' value");
      return;
    }
    const json& value = body->at("base_currency");
    std::optional<Id> id;
    const std::optional<std::string> text = encoded_text(value, id);
    if (!id) {
      if (!text || text->empty()) {
        respond_error(res, 400, "expected a base_currency id, code, or name");
        return;
      }
      id = service.resolve_lookup(LookupKind::Currency, lang, *text).id;
    }
    if (!service.set_base_currency(*id)) {
      respond_error(res, 422, "base_currency is not a currency entry");
      return;
    }
    respond(res, 200, settings_json(service, lang));
  });

  // List a vocabulary's entries, localized. :kind is one of the five lookup
  // kinds; an unknown kind is a 404.
  server.Get(R"(/lookups/([a-z]+))", [&service](const Request& req, Response& res) {
    const auto kind = coins::lookup_kind_from_string(req.matches[1].str());
    if (!kind) {
      respond_error(res, 404, "unknown lookup kind");
      return;
    }
    const std::string lang = lang_of(req);
    json entries = json::array();
    for (const LookupEntry& entry : service.lookups(*kind, lang)) {
      entries.push_back(lookup_entry_json(entry, lang));
    }
    respond(res, 200, entries);
  });

  // Resolve-or-create a vocabulary entry from {"name":..} or {"code":..}.
  server.Post(R"(/lookups/([a-z]+))", [&service](const Request& req, Response& res) {
    const auto kind = coins::lookup_kind_from_string(req.matches[1].str());
    if (!kind) {
      respond_error(res, 404, "unknown lookup kind");
      return;
    }
    const auto body = parse_body(req, res);
    if (!body) return;
    const std::string lang = lang_of(req);
    std::string text;
    if (body->contains("name") && body->at("name").is_string()) {
      text = body->at("name").get<std::string>();
    } else if (body->contains("code") && body->at("code").is_string()) {
      text = body->at("code").get<std::string>();
    }
    if (text.empty()) {
      respond_error(res, 400, "expected a non-empty 'name' or 'code'");
      return;
    }
    const LookupEntry entry = service.resolve_lookup(*kind, lang, text);
    respond(res, 201, lookup_entry_json(entry, lang));
  });

  // List a currency's units, localized.
  server.Get(R"(/currencies/(\d+)/units)", [&service](const Request& req, Response& res) {
    const Id currency_id = path_id(req);
    if (!service.lookup(currency_id)) {
      respond_error(res, 404, "currency not found");
      return;
    }
    const std::string lang = lang_of(req);
    json units = json::array();
    for (const CurrencyUnit& unit : service.currency_units(currency_id)) {
      units.push_back(currency_unit_json(unit, lang));
    }
    respond(res, 200, units);
  });

  // Resolve-or-create a currency unit from {"name":..}.
  server.Post(R"(/currencies/(\d+)/units)", [&service](const Request& req, Response& res) {
    const Id currency_id = path_id(req);
    if (!service.lookup(currency_id)) {
      respond_error(res, 404, "currency not found");
      return;
    }
    const auto body = parse_body(req, res);
    if (!body) return;
    const std::string lang = lang_of(req);
    std::string text;
    if (body->contains("name") && body->at("name").is_string()) {
      text = body->at("name").get<std::string>();
    } else if (body->contains("code") && body->at("code").is_string()) {
      text = body->at("code").get<std::string>();
    }
    if (text.empty()) {
      respond_error(res, 400, "expected a non-empty 'name' or 'code'");
      return;
    }
    const CurrencyUnit unit = service.resolve_currency_unit(currency_id, lang, text);
    respond(res, 201, currency_unit_json(unit, lang));
  });

  // Export (JSON by default, ?format=csv for CSV).
  server.Get("/export", [&service](const Request& req, Response& res) {
    if (req.get_param_value("format") == "csv") {
      res.status = 200;
      res.set_content(service.export_csv(), "text/csv");
    } else {
      res.status = 200;
      res.set_content(service.export_json(), "application/json");
    }
  });

  // Import a JSON collection (raw JSON body).
  server.Post("/import", [&service](const Request& req, Response& res) {
    const auto stats = service.import_json(req.body);
    if (!stats) {
      respond_validation(res, stats.error());
      return;
    }
    respond(res, 200,
            json{{"coins", stats->coins},
                 {"value_estimates", stats->value_estimates},
                 {"reference_links", stats->reference_links},
                 {"images", stats->images}});
  });
}

}  // namespace coins::server
