#include "coins/fs_image_store.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <string_view>
#include <system_error>

#include "coins/image_store_error.hpp"

namespace {

using coins::FilesystemImageStore;
using coins::ImageStoreError;

std::filesystem::path unique_temp_dir(std::string_view prefix) {
  std::random_device rd;
  const std::uint64_t token = (static_cast<std::uint64_t>(rd()) << 32) ^ rd();
  std::filesystem::path dir =
      std::filesystem::temp_directory_path() / (std::string{prefix} + std::to_string(token));
  std::filesystem::create_directories(dir);
  return dir;
}

class FilesystemImageStoreTest : public ::testing::Test {
 protected:
  void SetUp() override {
    root_ = unique_temp_dir("coins_store_");
    src_dir_ = unique_temp_dir("coins_src_");
  }
  void TearDown() override {
    std::error_code ec;
    std::filesystem::remove_all(root_, ec);
    std::filesystem::remove_all(src_dir_, ec);
  }

  std::filesystem::path write_source(std::string_view name, std::string_view bytes) {
    const std::filesystem::path path = src_dir_ / name;
    std::ofstream out(path, std::ios::binary);
    out << bytes;
    return path;
  }

  std::filesystem::path root_;
  std::filesystem::path src_dir_;
};

TEST_F(FilesystemImageStoreTest, StoresFileUnderCoinDirectory) {
  FilesystemImageStore store{root_};
  const std::filesystem::path source = write_source("obverse.png", "fake-png-bytes");

  const std::string stored = store.store(source, 42);
  EXPECT_TRUE(stored.starts_with("42/"));
  EXPECT_TRUE(stored.ends_with(".png"));
  EXPECT_TRUE(std::filesystem::is_regular_file(store.resolve(stored)));
}

TEST_F(FilesystemImageStoreTest, GeneratesUniqueNamesForRepeatedStores) {
  FilesystemImageStore store{root_};
  const std::filesystem::path source = write_source("x.jpg", "bytes");

  const std::string a = store.store(source, 7);
  const std::string b = store.store(source, 7);
  EXPECT_NE(a, b);
  EXPECT_TRUE(std::filesystem::is_regular_file(store.resolve(a)));
  EXPECT_TRUE(std::filesystem::is_regular_file(store.resolve(b)));
}

TEST_F(FilesystemImageStoreTest, RejectsMissingSource) {
  FilesystemImageStore store{root_};
  EXPECT_THROW((void)store.store(src_dir_ / "nope.png", 1), ImageStoreError);
}

TEST_F(FilesystemImageStoreTest, RejectsUnsupportedType) {
  FilesystemImageStore store{root_};
  const std::filesystem::path source = write_source("notes.txt", "hello");
  EXPECT_THROW((void)store.store(source, 1), ImageStoreError);
}

TEST_F(FilesystemImageStoreTest, RemoveDeletesStoredFile) {
  FilesystemImageStore store{root_};
  const std::filesystem::path source = write_source("y.webp", "bytes");
  const std::string stored = store.store(source, 3);

  EXPECT_TRUE(store.remove(stored));
  EXPECT_FALSE(std::filesystem::exists(store.resolve(stored)));
  EXPECT_FALSE(store.remove(stored));  // already gone
}

TEST(ImageExtensionTest, RecognizesSupportedTypes) {
  for (const auto* ext : {".png", ".jpg", ".jpeg", ".gif", ".webp", ".PNG", "JPG"}) {
    EXPECT_TRUE(coins::is_supported_image_extension(ext)) << ext;
  }
  for (const auto* ext : {".txt", ".pdf", ".bmp", ""}) {
    EXPECT_FALSE(coins::is_supported_image_extension(ext)) << ext;
  }
}

}  // namespace
