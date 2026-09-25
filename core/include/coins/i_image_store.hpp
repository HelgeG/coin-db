#ifndef COINS_I_IMAGE_STORE_HPP
#define COINS_I_IMAGE_STORE_HPP

#include <filesystem>
#include <string>
#include <string_view>

#include "coins/id.hpp"

namespace coins {

/// The image file extensions the store accepts (lower-case, leading dot).
[[nodiscard]] bool is_supported_image_extension(std::string_view extension);

/// Managed store for coin image files. Abstracts *where* and *how* image bytes
/// are persisted so the rest of the core depends only on stored relative paths
/// (dependency inversion). The default implementation is filesystem-backed.
class IImageStore {
 public:
  virtual ~IImageStore() = default;

  /// Copies `source` into the store for `coin_id`, returning the stored path
  /// relative to the store root (e.g. "42/ab12...cd.png"). The copy is atomic
  /// (temp file + rename). Throws `ImageStoreError` on I/O failure. The caller
  /// is expected to have validated that `source` exists and is a supported
  /// image type.
  [[nodiscard]] virtual std::string store(const std::filesystem::path& source, Id coin_id) = 0;

  /// Removes the file at `stored_path` (relative to the store root). Returns
  /// whether a file was removed.
  virtual bool remove(std::string_view stored_path) = 0;

  /// Resolves a stored relative path to its absolute filesystem path.
  [[nodiscard]] virtual std::filesystem::path resolve(std::string_view stored_path) const = 0;
};

}  // namespace coins

#endif  // COINS_I_IMAGE_STORE_HPP
