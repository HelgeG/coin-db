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
#include "coins/id.hpp"
#include "coins/image.hpp"
#include "coins/json.hpp"
#include "coins/reference_link.hpp"
#include "coins/validation.hpp"
#include "coins/value_estimate.hpp"

namespace coins::server {
namespace {

using json = nlohmann::json;
using httplib::Request;
using httplib::Response;

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
    json coins = json::array();
    for (const Coin& coin : service.search(query_from_request(req))) {
      coins.push_back(coin_to_json(coin));
    }
    respond(res, 200, coins);
  });

  // Create.
  server.Post("/coins", [&service](const Request& req, Response& res) {
    const auto body = parse_body(req, res);
    if (!body) return;
    const auto created = service.add_coin(coin_from_json(*body));
    if (!created) {
      respond_validation(res, created.error());
      return;
    }
    respond(res, 201, coin_to_json(*created));
  });

  // Get one, with relations.
  server.Get(R"(/coins/(\d+))", [&service](const Request& req, Response& res) {
    const Id id = path_id(req);
    const auto coin = service.get_coin(id);
    if (!coin) {
      respond_error(res, 404, "coin not found");
      return;
    }
    json body = coin_to_json(*coin);
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
    Coin coin = coin_from_json(*body);
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
    respond(res, 200, coin_to_json(*service.get_coin(id)));
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
    // Unknown or missing ?type= falls back to the default summary.
    coins::SummaryType type = coins::kDefaultSummaryType;
    if (req.has_param("type")) {
      if (const auto parsed = coins::summary_type_from_string(req.get_param_value("type"))) {
        type = *parsed;
      }
    }
    respond(res, 200, summary_to_json(service.summary(type)));
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
