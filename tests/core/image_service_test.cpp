#include "coins/image_service.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <system_error>

#include "coins/db/database.hpp"
#include "coins/db/database_error.hpp"
#include "coins/db/schema.hpp"
#include "coins/db/statement.hpp"
#include "coins/fs_image_store.hpp"
#include "coins/id.hpp"
#include "coins/image.hpp"

namespace {

using coins::FilesystemImageStore;
using coins::Id;
using coins::ImageKind;
using coins::ImageService;
using coins::db::Database;

std::filesystem::path unique_temp_dir(std::string_view prefix) {
  std::random_device rd;
  const std::uint64_t token = (static_cast<std::uint64_t>(rd()) << 32) ^ rd();
  std::filesystem::path dir =
      std::filesystem::temp_directory_path() / (std::string{prefix} + std::to_string(token));
  std::filesystem::create_directories(dir);
  return dir;
}

std::size_t count_files(const std::filesystem::path& dir) {
  std::error_code ec;
  if (!std::filesystem::exists(dir, ec)) {
    return 0;
  }
  std::size_t count = 0;
  for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
    if (entry.is_regular_file()) {
      ++count;
    }
  }
  return count;
}

class ImageServiceTest : public ::testing::Test {
 protected:
  ImageServiceTest()
      : root_(unique_temp_dir("coins_imgsvc_")),
        src_dir_(unique_temp_dir("coins_imgsvc_src_")),
        db_(Database::in_memory()),
        store_(root_) {
    coins::db::bootstrap_schema(db_);
    coin_id_ = insert_coin();
  }
  ~ImageServiceTest() override {
    std::error_code ec;
    std::filesystem::remove_all(root_, ec);
    std::filesystem::remove_all(src_dir_, ec);
  }

  Id insert_coin() {
    coins::db::Statement stmt = db_.prepare(
        "INSERT INTO coin (country, year_from, year_to, created_at, updated_at) "
        "VALUES ('Norway', 1963, 1963, '2026-01-01', '2026-01-01');");
    (void)stmt.step();
    return db_.last_insert_rowid();
  }

  std::filesystem::path write_source(std::string_view name, std::string_view bytes) {
    const std::filesystem::path path = src_dir_ / name;
    std::ofstream out(path, std::ios::binary);
    out << bytes;
    return path;
  }

  ImageService service() { return ImageService{db_, store_}; }

  std::filesystem::path root_;
  std::filesystem::path src_dir_;
  Database db_;
  FilesystemImageStore store_;
  Id coin_id_ = 0;
};

TEST_F(ImageServiceTest, AddImageStoresFileAndRow) {
  ImageService svc = service();
  const std::filesystem::path source = write_source("obverse.png", "bytes");

  const auto added = svc.add_image(coin_id_, source, ImageKind::Obverse, std::string{"front"});
  ASSERT_TRUE(added.has_value());
  EXPECT_GT(added->id, 0);
  EXPECT_TRUE(std::filesystem::is_regular_file(store_.resolve(added->stored_path)));

  const auto images = svc.list_images(coin_id_);
  ASSERT_EQ(images.size(), 1U);
  EXPECT_EQ(images[0].kind, ImageKind::Obverse);
  EXPECT_EQ(images[0].original_name, std::optional<std::string>{"obverse.png"});
  EXPECT_EQ(images[0].caption, std::optional<std::string>{"front"});
}

TEST_F(ImageServiceTest, AddImageMissingSourceReturnsError) {
  ImageService svc = service();
  const auto added = svc.add_image(coin_id_, src_dir_ / "missing.png", std::nullopt, std::nullopt);
  EXPECT_FALSE(added.has_value());
  EXPECT_TRUE(svc.list_images(coin_id_).empty());
}

TEST_F(ImageServiceTest, AddImageUnsupportedTypeReturnsError) {
  ImageService svc = service();
  const std::filesystem::path source = write_source("scan.txt", "hello");
  const auto added = svc.add_image(coin_id_, source, std::nullopt, std::nullopt);
  EXPECT_FALSE(added.has_value());
}

TEST_F(ImageServiceTest, RemoveImageDeletesRowAndFile) {
  ImageService svc = service();
  const std::filesystem::path source = write_source("r.jpg", "bytes");
  const auto added = svc.add_image(coin_id_, source, ImageKind::Reverse, std::nullopt);
  ASSERT_TRUE(added.has_value());

  EXPECT_TRUE(svc.remove_image(added->id));
  EXPECT_FALSE(std::filesystem::exists(store_.resolve(added->stored_path)));
  EXPECT_TRUE(svc.list_images(coin_id_).empty());
  EXPECT_FALSE(svc.remove_image(added->id));  // already gone
}

TEST_F(ImageServiceTest, PurgeCoinImagesRemovesAllRowsAndFiles) {
  ImageService svc = service();
  ASSERT_TRUE(
      svc.add_image(coin_id_, write_source("a.png", "b"), std::nullopt, std::nullopt).has_value());
  ASSERT_TRUE(
      svc.add_image(coin_id_, write_source("b.png", "b"), std::nullopt, std::nullopt).has_value());

  svc.purge_coin_images(coin_id_);
  EXPECT_TRUE(svc.list_images(coin_id_).empty());
  EXPECT_EQ(count_files(root_ / std::to_string(coin_id_)), 0U);
}

TEST_F(ImageServiceTest, AddImageRollsBackFileWhenRowInsertFails) {
  ImageService svc = service();
  const std::filesystem::path source = write_source("c.png", "bytes");

  // coin_id 9999 does not exist -> FK violation on insert -> DatabaseError; the
  // copied file must be rolled back rather than orphaned.
  constexpr Id kMissingCoin = 9999;
  EXPECT_THROW((void)svc.add_image(kMissingCoin, source, std::nullopt, std::nullopt),
               coins::db::DatabaseError);
  EXPECT_EQ(count_files(root_ / std::to_string(kMissingCoin)), 0U);
}

}  // namespace
