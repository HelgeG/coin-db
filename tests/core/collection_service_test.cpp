#include "coins/collection_service.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <string_view>
#include <system_error>

#include "coins/coin.hpp"
#include "coins/image.hpp"
#include "coins/lookup_kind.hpp"
#include "coins/value_estimate.hpp"

namespace {

using coins::Coin;
using coins::CollectionService;
using coins::ImageKind;
using coins::LookupKind;

std::filesystem::path unique_temp_dir(std::string_view prefix) {
  std::random_device rd;
  const std::uint64_t token = (static_cast<std::uint64_t>(rd()) << 32) ^ rd();
  std::filesystem::path dir =
      std::filesystem::temp_directory_path() / (std::string{prefix} + std::to_string(token));
  std::filesystem::create_directories(dir);
  return dir;
}

class CollectionServiceTest : public ::testing::Test {
 protected:
  CollectionServiceTest()
      : data_dir_(unique_temp_dir("coins_svc_")),
        src_dir_(unique_temp_dir("coins_svc_src_")),
        service_(CollectionService::from_data_dir(data_dir_)) {}
  ~CollectionServiceTest() override {
    std::error_code ec;
    std::filesystem::remove_all(data_dir_, ec);
    std::filesystem::remove_all(src_dir_, ec);
  }

  Coin valid_coin() {
    Coin coin;
    coin.country_id = service_.resolve_lookup(LookupKind::Country, "en", "Norway").id;
    coin.year_from = 1963;
    coin.year_to = 1963;
    return coin;
  }

  std::filesystem::path write_png(std::string_view name) {
    const std::filesystem::path path = src_dir_ / name;
    std::ofstream out(path, std::ios::binary);
    out << "bytes";
    return path;
  }

  std::filesystem::path data_dir_;
  std::filesystem::path src_dir_;
  CollectionService service_;
};

TEST_F(CollectionServiceTest, AddAndGetCoin) {
  const Coin coin = valid_coin();
  const auto created = service_.add_coin(coin);
  ASSERT_TRUE(created.has_value());
  const auto fetched = service_.get_coin(created->id);
  ASSERT_TRUE(fetched.has_value());
  EXPECT_EQ(fetched->country_id, coin.country_id);
  // The resolved country entry displays as "Norway" in English.
  const auto entry = service_.lookup(fetched->country_id);
  ASSERT_TRUE(entry.has_value());
  EXPECT_EQ(entry->display_name("en"), "Norway");
}

TEST_F(CollectionServiceTest, DeleteCoinPurgesImageFilesAndRows) {
  const auto coin = service_.add_coin(valid_coin());
  ASSERT_TRUE(coin.has_value());

  const auto image =
      service_.add_image(coin->id, write_png("obv.png"), ImageKind::Obverse, std::nullopt);
  ASSERT_TRUE(image.has_value());
  const std::filesystem::path stored = service_.resolve_image(image->stored_path);
  ASSERT_TRUE(std::filesystem::is_regular_file(stored));

  EXPECT_TRUE(service_.delete_coin(coin->id));

  EXPECT_FALSE(service_.get_coin(coin->id).has_value());
  EXPECT_TRUE(service_.images(coin->id).empty());
  EXPECT_FALSE(std::filesystem::exists(stored));  // file purged, not orphaned
}

TEST_F(CollectionServiceTest, DeleteMissingCoinReturnsFalse) {
  EXPECT_FALSE(service_.delete_coin(999));
}

TEST_F(CollectionServiceTest, EstimateAndSummaryFlow) {
  const auto coin = service_.add_coin(valid_coin());
  ASSERT_TRUE(coin.has_value());

  coins::ValueEstimate est;
  est.coin_id = coin->id;
  est.amount_eur = 42.0;
  est.estimated_at = "2026-01-01";
  ASSERT_TRUE(service_.add_estimate(est).has_value());

  const auto summary = service_.summary();
  EXPECT_EQ(summary.coin_count, 1);
  EXPECT_DOUBLE_EQ(summary.total_estimate_eur, 42.0);
}

TEST_F(CollectionServiceTest, JsonExportImportRoundTripAcrossServices) {
  const auto coin = service_.add_coin(valid_coin());
  ASSERT_TRUE(coin.has_value());
  const std::string exported = service_.export_json();

  CollectionService restored = CollectionService::from_data_dir(unique_temp_dir("coins_svc2_"));
  const auto stats = restored.import_json(exported);
  ASSERT_TRUE(stats.has_value());
  EXPECT_EQ(stats->coins, 1);
  EXPECT_TRUE(restored.get_coin(coin->id).has_value());
}

}  // namespace
