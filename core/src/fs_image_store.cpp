#include "coins/fs_image_store.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <format>
#include <random>
#include <string>
#include <system_error>
#include <utility>

#include "coins/image_store_error.hpp"

namespace coins {
namespace {

std::string to_lower(std::string text) {
  std::ranges::transform(text, text.begin(),
                         [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return text;
}

// 32 hex chars from 128 random bits. Collision-safe enough for file naming.
std::string generate_uuid_hex() {
  static thread_local std::mt19937_64 rng{std::random_device{}()};
  std::uniform_int_distribution<std::uint64_t> dist;
  return std::format("{:016x}{:016x}", dist(rng), dist(rng));
}

}  // namespace

bool is_supported_image_extension(std::string_view extension) {
  static constexpr std::array<std::string_view, 5> kSupported{".png", ".jpg", ".jpeg", ".gif",
                                                              ".webp"};
  std::string normalized = to_lower(std::string{extension});
  if (!normalized.empty() && normalized.front() != '.') {
    normalized.insert(normalized.begin(), '.');
  }
  return std::ranges::find(kSupported, normalized) != kSupported.end();
}

FilesystemImageStore::FilesystemImageStore(std::filesystem::path root) : root_(std::move(root)) {}

std::string FilesystemImageStore::store(const std::filesystem::path& source, Id coin_id) {
  std::error_code ec;
  if (!std::filesystem::is_regular_file(source, ec) || ec) {
    throw ImageStoreError("image source is not a readable file: " + source.string());
  }

  const std::string extension = to_lower(source.extension().string());
  if (!is_supported_image_extension(extension)) {
    throw ImageStoreError("unsupported image type: " + extension);
  }

  const std::filesystem::path coin_dir = root_ / std::to_string(coin_id);
  std::filesystem::create_directories(coin_dir, ec);
  if (ec) {
    throw ImageStoreError("could not create image directory: " + coin_dir.string());
  }

  const std::string filename = generate_uuid_hex() + extension;
  const std::filesystem::path destination = coin_dir / filename;
  std::filesystem::path temp = destination;
  temp += ".tmp";

  // Copy to a temp file in the destination directory, then rename into place so
  // a reader never observes a partially written file.
  std::filesystem::copy_file(source, temp, std::filesystem::copy_options::overwrite_existing, ec);
  if (ec) {
    throw ImageStoreError("failed to copy image: " + ec.message());
  }
  std::filesystem::rename(temp, destination, ec);
  if (ec) {
    std::error_code cleanup_ec;
    std::filesystem::remove(temp, cleanup_ec);
    throw ImageStoreError("failed to finalize image: " + ec.message());
  }

  return std::format("{}/{}", coin_id, filename);
}

bool FilesystemImageStore::remove(std::string_view stored_path) {
  std::error_code ec;
  const bool removed = std::filesystem::remove(resolve(stored_path), ec);
  return removed && !ec;
}

std::filesystem::path FilesystemImageStore::resolve(std::string_view stored_path) const {
  return root_ / std::filesystem::path{stored_path};
}

}  // namespace coins
