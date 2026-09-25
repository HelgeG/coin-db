#ifndef COINS_FS_IMAGE_STORE_HPP
#define COINS_FS_IMAGE_STORE_HPP

#include <filesystem>
#include <string>
#include <string_view>

#include "coins/i_image_store.hpp"
#include "coins/id.hpp"

namespace coins {

/// Filesystem-backed image store rooted at a configured directory. Files are
/// laid out as `<root>/<coin_id>/<uuid>.<ext>`; the returned stored path is
/// relative to `<root>` and uses '/' separators. Writes are atomic (the bytes
/// are copied to a temporary file in the destination directory, then renamed).
class FilesystemImageStore final : public IImageStore {
 public:
  explicit FilesystemImageStore(std::filesystem::path root);

  [[nodiscard]] std::string store(const std::filesystem::path& source, Id coin_id) override;
  bool remove(std::string_view stored_path) override;
  [[nodiscard]] std::filesystem::path resolve(std::string_view stored_path) const override;

  [[nodiscard]] const std::filesystem::path& root() const noexcept { return root_; }

 private:
  std::filesystem::path root_;
};

}  // namespace coins

#endif  // COINS_FS_IMAGE_STORE_HPP
