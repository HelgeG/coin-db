#ifndef COINS_IMAGE_STORE_ERROR_HPP
#define COINS_IMAGE_STORE_ERROR_HPP

#include <stdexcept>
#include <string>

namespace coins {

/// Thrown for exceptional image-store failures (I/O errors during copy/rename,
/// missing store root, etc.). Expected, user-facing preconditions such as "file
/// does not exist" or "unsupported image type" are reported via validation
/// results by higher-level operations, not thrown.
class ImageStoreError : public std::runtime_error {
 public:
  explicit ImageStoreError(const std::string& message) : std::runtime_error(message) {}
};

}  // namespace coins

#endif  // COINS_IMAGE_STORE_ERROR_HPP
