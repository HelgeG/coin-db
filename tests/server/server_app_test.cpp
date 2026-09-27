#include "server_app.hpp"

#include <gtest/gtest.h>
#include <httplib.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>

#include "coins/collection_service.hpp"

namespace {

using json = nlohmann::json;

std::filesystem::path unique_temp_dir(std::string_view prefix) {
  std::random_device rd;
  const std::uint64_t token = (static_cast<std::uint64_t>(rd()) << 32) ^ rd();
  std::filesystem::path dir =
      std::filesystem::temp_directory_path() / (std::string{prefix} + std::to_string(token));
  std::filesystem::create_directories(dir);
  return dir;
}

class ServerAppTest : public ::testing::Test {
 protected:
  void SetUp() override {
    data_dir_ = unique_temp_dir("coins_srv_");
    service_ =
        std::make_unique<coins::CollectionService>(data_dir_ / "coins.db", data_dir_ / "images");
    coins::server::register_routes(server_, *service_);

    port_ = server_.bind_to_any_port("127.0.0.1");
    ASSERT_GT(port_, 0);
    listener_ = std::thread([this] { server_.listen_after_bind(); });
    while (!server_.is_running()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  void TearDown() override {
    server_.stop();
    if (listener_.joinable()) listener_.join();
    std::error_code ec;
    std::filesystem::remove_all(data_dir_, ec);
  }

  httplib::Client client() { return httplib::Client{"127.0.0.1", port_}; }

  // Creates a minimal valid coin via the API and returns its id. `country` is
  // sent as a name string, which the server resolves against the seeded
  // vocabulary (reusing the "Norway" / NO entry).
  coins::Id create_coin() {
    httplib::Client cli = client();
    const json body = {{"country", "Norway"}, {"year_from", 1963}, {"year_to", 1963}};
    const auto res = cli.Post("/coins", body.dump(), "application/json");
    EXPECT_TRUE(res);
    EXPECT_EQ(res->status, 201);
    return json::parse(res->body).at("id").get<coins::Id>();
  }

  // Looks up a lookup entry id by code via GET /lookups/:kind.
  coins::Id lookup_id(const std::string& kind, const std::string& code) {
    httplib::Client cli = client();
    const auto res = cli.Get("/lookups/" + kind);
    EXPECT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    for (const json& entry : json::parse(res->body)) {
      if (entry.at("code").get<std::string>() == code) {
        return entry.at("id").get<coins::Id>();
      }
    }
    ADD_FAILURE() << "no " << kind << " entry with code " << code;
    return 0;
  }

  std::filesystem::path data_dir_;
  std::unique_ptr<coins::CollectionService> service_;
  httplib::Server server_;
  std::thread listener_;
  int port_ = 0;
};

TEST_F(ServerAppTest, CreateAndGetCoin) {
  const coins::Id id = create_coin();

  httplib::Client cli = client();
  const auto res = cli.Get("/coins/" + std::to_string(id));
  ASSERT_TRUE(res);
  EXPECT_EQ(res->status, 200);
  const json body = json::parse(res->body);
  // Encoded fields serialize as nested {id,code,name} objects (localized).
  const json country = body.at("country");
  EXPECT_EQ(country.at("code").get<std::string>(), "NO");
  EXPECT_EQ(country.at("name").get<std::string>(), "Norway");
  EXPECT_GT(country.at("id").get<coins::Id>(), 0);
  // Unset encoded fields are null.
  EXPECT_TRUE(body.at("denomination").is_null());
  EXPECT_TRUE(body.at("currency").is_null());
  EXPECT_TRUE(body.at("face_unit").is_null());
  EXPECT_TRUE(body.at("value_estimates").is_array());
  EXPECT_TRUE(body.at("reference_links").is_array());
  EXPECT_TRUE(body.at("images").is_array());
}

TEST_F(ServerAppTest, CoinCountryLocalizedByLangParam) {
  const coins::Id id = create_coin();
  httplib::Client cli = client();
  // ?lang=nb returns the Norwegian display name for the same entry.
  const auto res = cli.Get("/coins/" + std::to_string(id) + "?lang=nb");
  ASSERT_TRUE(res);
  EXPECT_EQ(res->status, 200);
  const json country = json::parse(res->body).at("country");
  EXPECT_EQ(country.at("code").get<std::string>(), "NO");
  EXPECT_EQ(country.at("name").get<std::string>(), "Norge");
}

TEST_F(ServerAppTest, CreateCoinWithCurrencyAndFaceUnit) {
  const coins::Id nok_id = lookup_id("currency", "NOK");
  // Find the NOK "øre" unit id.
  httplib::Client cli = client();
  const auto units = cli.Get("/currencies/" + std::to_string(nok_id) + "/units");
  ASSERT_TRUE(units);
  EXPECT_EQ(units->status, 200);
  coins::Id ore_id = 0;
  for (const json& u : json::parse(units->body)) {
    if (u.at("code").get<std::string>() == "ore") ore_id = u.at("id").get<coins::Id>();
  }
  ASSERT_GT(ore_id, 0);

  // Create a coin referencing the currency by id and the unit by id.
  const json body = {{"country", "Norway"}, {"year_from", 1963}, {"year_to", 1963},
                     {"currency", nok_id},  {"face_value", 50},  {"face_unit", ore_id}};
  const auto created = cli.Post("/coins", body.dump(), "application/json");
  ASSERT_TRUE(created);
  EXPECT_EQ(created->status, 201);
  const json out = json::parse(created->body);
  EXPECT_EQ(out.at("currency").at("code").get<std::string>(), "NOK");
  EXPECT_DOUBLE_EQ(out.at("face_value").get<double>(), 50.0);
  EXPECT_EQ(out.at("face_unit").at("code").get<std::string>(), "ore");
  EXPECT_EQ(out.at("face_unit").at("name").get<std::string>(), "øre");
}

TEST_F(ServerAppTest, CreateInvalidReturns422) {
  httplib::Client cli = client();
  const auto res = cli.Post("/coins", "{}", "application/json");
  ASSERT_TRUE(res);
  EXPECT_EQ(res->status, 422);
  EXPECT_TRUE(json::parse(res->body).at("errors").is_array());
}

TEST_F(ServerAppTest, GetMissingCoinReturns404) {
  httplib::Client cli = client();
  const auto res = cli.Get("/coins/9999");
  ASSERT_TRUE(res);
  EXPECT_EQ(res->status, 404);
}

TEST_F(ServerAppTest, ListReturnsAllCoins) {
  create_coin();
  create_coin();
  httplib::Client cli = client();
  const auto res = cli.Get("/coins");
  ASSERT_TRUE(res);
  EXPECT_EQ(res->status, 200);
  EXPECT_EQ(json::parse(res->body).size(), 2U);
}

TEST_F(ServerAppTest, AddEstimateThenSummary) {
  const coins::Id id = create_coin();
  httplib::Client cli = client();

  const json estimate = {{"amount_eur", 100.0}, {"estimated_at", "2026-01-01"}};
  const auto add =
      cli.Post("/coins/" + std::to_string(id) + "/estimates", estimate.dump(), "application/json");
  ASSERT_TRUE(add);
  EXPECT_EQ(add->status, 201);

  // total_value gives the headline figures with no breakdown buckets (this does
  // not depend on the localized-grouping summary work).
  const auto summary = cli.Get("/summary?type=total_value");
  ASSERT_TRUE(summary);
  EXPECT_EQ(summary->status, 200);
  const json body = json::parse(summary->body);
  EXPECT_EQ(body.at("coin_count").get<int>(), 1);
  EXPECT_DOUBLE_EQ(body.at("total_estimate_eur").get<double>(), 100.0);
  const json breakdown = body.at("breakdown");
  EXPECT_EQ(breakdown.at("type").get<std::string>(), "total_value");
  EXPECT_TRUE(breakdown.at("buckets").empty());
}

TEST_F(ServerAppTest, ListLookupsCountryLocalized) {
  httplib::Client cli = client();

  // English (default): the seeded NO entry is "Norway".
  const auto en = cli.Get("/lookups/country");
  ASSERT_TRUE(en);
  EXPECT_EQ(en->status, 200);
  bool found_en = false;
  for (const json& entry : json::parse(en->body)) {
    if (entry.at("code").get<std::string>() == "NO") {
      EXPECT_EQ(entry.at("name").get<std::string>(), "Norway");
      found_en = true;
    }
  }
  EXPECT_TRUE(found_en);

  // Norwegian: the same entry displays as "Norge".
  const auto nb = cli.Get("/lookups/country?lang=nb");
  ASSERT_TRUE(nb);
  EXPECT_EQ(nb->status, 200);
  bool found_nb = false;
  for (const json& entry : json::parse(nb->body)) {
    if (entry.at("code").get<std::string>() == "NO") {
      EXPECT_EQ(entry.at("name").get<std::string>(), "Norge");
      found_nb = true;
    }
  }
  EXPECT_TRUE(found_nb);
}

TEST_F(ServerAppTest, UnknownLookupKindReturns404) {
  httplib::Client cli = client();
  const auto res = cli.Get("/lookups/nope");
  ASSERT_TRUE(res);
  EXPECT_EQ(res->status, 404);
}

TEST_F(ServerAppTest, PostLookupResolvesOrCreates) {
  httplib::Client cli = client();

  // Creating a new composition (app-generated code) returns {id,code,name}.
  const json body = {{"name", "Bronze"}};
  const auto created = cli.Post("/lookups/composition", body.dump(), "application/json");
  ASSERT_TRUE(created);
  EXPECT_EQ(created->status, 201);
  const json first = json::parse(created->body);
  EXPECT_EQ(first.at("name").get<std::string>(), "Bronze");
  EXPECT_FALSE(first.at("code").get<std::string>().empty());

  // Resolving the same name again (case-insensitive) reuses the entry.
  const json again_body = {{"name", "bronze"}};
  const auto again = cli.Post("/lookups/composition", again_body.dump(), "application/json");
  ASSERT_TRUE(again);
  EXPECT_EQ(again->status, 201);
  EXPECT_EQ(json::parse(again->body).at("id").get<coins::Id>(), first.at("id").get<coins::Id>());
}

TEST_F(ServerAppTest, CurrencyUnitsListedAndCreated) {
  const coins::Id nok_id = lookup_id("currency", "NOK");
  httplib::Client cli = client();

  // NOK is seeded with krone (major) + øre.
  const auto list = cli.Get("/currencies/" + std::to_string(nok_id) + "/units");
  ASSERT_TRUE(list);
  EXPECT_EQ(list->status, 200);
  const json units = json::parse(list->body);
  EXPECT_GE(units.size(), 2u);
  bool has_krone = false;
  bool has_ore = false;
  for (const json& u : units) {
    const std::string code = u.at("code").get<std::string>();
    if (code == "krone") has_krone = true;
    if (code == "ore") has_ore = true;
  }
  EXPECT_TRUE(has_krone);
  EXPECT_TRUE(has_ore);

  // The øre unit localizes to "øre".
  const auto nb = cli.Get("/currencies/" + std::to_string(nok_id) + "/units?lang=nb");
  ASSERT_TRUE(nb);
  for (const json& u : json::parse(nb->body)) {
    if (u.at("code").get<std::string>() == "ore") {
      EXPECT_EQ(u.at("name").get<std::string>(), "øre");
    }
  }
}

TEST_F(ServerAppTest, SummaryTypeSelectsBreakdown) {
  create_coin();
  httplib::Client cli = client();

  // A recognized type is honored. (by_decade groups on year columns only, so it
  // does not depend on the localized lookup-grouping summary work in core.)
  const auto by_decade = cli.Get("/summary?type=by_decade");
  ASSERT_TRUE(by_decade);
  EXPECT_EQ(by_decade->status, 200);
  EXPECT_EQ(json::parse(by_decade->body).at("breakdown").at("type").get<std::string>(),
            "by_decade");

  // total_value carries no buckets.
  const auto total = cli.Get("/summary?type=total_value");
  ASSERT_TRUE(total);
  const json total_breakdown = json::parse(total->body).at("breakdown");
  EXPECT_EQ(total_breakdown.at("type").get<std::string>(), "total_value");
  EXPECT_TRUE(total_breakdown.at("buckets").empty());
}

TEST_F(ServerAppTest, DeleteCoin) {
  const coins::Id id = create_coin();
  httplib::Client cli = client();

  const auto del = cli.Delete("/coins/" + std::to_string(id));
  ASSERT_TRUE(del);
  EXPECT_EQ(del->status, 200);

  const auto get = cli.Get("/coins/" + std::to_string(id));
  ASSERT_TRUE(get);
  EXPECT_EQ(get->status, 404);
}

TEST_F(ServerAppTest, AddLinkWithInvalidUrlReturns422) {
  const coins::Id id = create_coin();
  httplib::Client cli = client();
  const json link = {{"label", "Numista"}, {"url", "not-a-url"}};
  const auto res =
      cli.Post("/coins/" + std::to_string(id) + "/links", link.dump(), "application/json");
  ASSERT_TRUE(res);
  EXPECT_EQ(res->status, 422);
}

TEST_F(ServerAppTest, UploadImageMultipartThenServeBytes) {
  const coins::Id id = create_coin();
  httplib::Client cli = client();

  const httplib::UploadFormDataItems items = {
      {"file", "fake-png-bytes", "obverse.png", "image/png"}, {"kind", "obverse", "", ""}};
  const auto res = cli.Post("/coins/" + std::to_string(id) + "/images", items);
  ASSERT_TRUE(res);
  EXPECT_EQ(res->status, 201);
  const json created = json::parse(res->body);
  const coins::Id image_id = created.at("id").get<coins::Id>();
  EXPECT_GT(image_id, 0);
  EXPECT_EQ(created.at("original_name").get<std::string>(), "obverse.png");

  const auto get = cli.Get("/coins/" + std::to_string(id));
  ASSERT_TRUE(get);
  EXPECT_EQ(json::parse(get->body).at("images").size(), 1U);

  // The stored bytes are served back with an image content type.
  const auto file = cli.Get("/images/" + std::to_string(image_id) + "/file");
  ASSERT_TRUE(file);
  EXPECT_EQ(file->status, 200);
  EXPECT_EQ(file->body, "fake-png-bytes");
  EXPECT_EQ(file->get_header_value("Content-Type"), "image/png");
}

TEST_F(ServerAppTest, ServeMissingImageReturns404) {
  httplib::Client cli = client();
  const auto file = cli.Get("/images/9999/file");
  ASSERT_TRUE(file);
  EXPECT_EQ(file->status, 404);
}

TEST_F(ServerAppTest, ExportReturnsJsonCollection) {
  create_coin();
  httplib::Client cli = client();
  const auto res = cli.Get("/export");
  ASSERT_TRUE(res);
  EXPECT_EQ(res->status, 200);
  EXPECT_TRUE(json::parse(res->body).at("coins").is_array());
}

}  // namespace
