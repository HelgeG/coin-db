#include "coins/image.hpp"

namespace coins {

std::string_view to_string(ImageKind kind) noexcept {
  switch (kind) {
    case ImageKind::Obverse:
      return "obverse";
    case ImageKind::Reverse:
      return "reverse";
    case ImageKind::Detail:
      return "detail";
  }
  return "detail";  // unreachable for a valid enum value; keeps the compiler happy
}

std::optional<ImageKind> image_kind_from_string(std::string_view text) {
  if (text == "obverse") {
    return ImageKind::Obverse;
  }
  if (text == "reverse") {
    return ImageKind::Reverse;
  }
  if (text == "detail") {
    return ImageKind::Detail;
  }
  return std::nullopt;
}

}  // namespace coins
