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

  // Creates a minimal valid coin via the API and returns its id.
  coins::Id create_coin() {
    httplib::Client cli = client();
    const json body = {{"country", "Norway"}, {"year_from", 1963}, {"year_to", 1963}};
    const auto res = cli.Post("/coins", body.dump(), "application/json");
    EXPECT_TRUE(res);
    EXPECT_EQ(res->status, 201);
    return json::parse(res->body).at("id").get<coins::Id>();
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
  EXPECT_EQ(body.at("country").get<std::string>(), "Norway");
  EXPECT_TRUE(body.at("value_estimates").is_array());
  EXPECT_TRUE(body.at("reference_links").is_array());
  EXPECT_TRUE(body.at("images").is_array());
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

  const auto summary = cli.Get("/summary");
  ASSERT_TRUE(summary);
  EXPECT_EQ(summary->status, 200);
  const json body = json::parse(summary->body);
  EXPECT_EQ(body.at("coin_count").get<int>(), 1);
  EXPECT_DOUBLE_EQ(body.at("total_estimate_eur").get<double>(), 100.0);
  const json countries = body.at("coins_by_country");
  ASSERT_EQ(countries.size(), 1u);
  EXPECT_EQ(countries[0].at("country").get<std::string>(), "Norway");
  EXPECT_EQ(countries[0].at("coin_count").get<int>(), 1);
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
