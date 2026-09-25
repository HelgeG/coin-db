#ifndef COINS_IMAGE_HPP
#define COINS_IMAGE_HPP

#include <optional>
#include <string>
#include <string_view>

#include "coins/id.hpp"

namespace coins {

/// Which face or aspect of the coin an image depicts.
enum class ImageKind { Obverse, Reverse, Detail };

/// Canonical lower-case token for an image kind (e.g. "obverse"). Stored in the
/// `image.kind` column and used in the CLI/REST interfaces.
[[nodiscard]] std::string_view to_string(ImageKind kind) noexcept;

/// Parses an image-kind token. Returns `std::nullopt` for an unrecognized
/// value, so callers can decide whether that is an error.
[[nodiscard]] std::optional<ImageKind> image_kind_from_string(std::string_view text);

/// An image attached to a coin: one row of the `image` table. The source file
/// is copied into the managed image store; `stored_path` is the path relative
/// to that store's root (never the original external path).
struct Image {
  Id id = kUnsavedId;
  Id coin_id = kUnsavedId;

  std::optional<ImageKind> kind;
  std::string stored_path;                   // relative to the image-store root
  std::optional<std::string> original_name;  // original filename, for reference
  std::optional<std::string> caption;
};

}  // namespace coins

#endif  // COINS_IMAGE_HPP
