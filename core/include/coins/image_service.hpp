#ifndef COINS_IMAGE_SERVICE_HPP
#define COINS_IMAGE_SERVICE_HPP

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "coins/i_image_store.hpp"
#include "coins/id.hpp"
#include "coins/image.hpp"
#include "coins/validation.hpp"

namespace coins::db {
class Database;
}

namespace coins {

/// Coordinates image persistence: the `image` table rows and the managed image
/// files kept in an `IImageStore`. Keeping both sides behind one component
/// ensures rows and files stay consistent (a file is copied in before its row
/// is written, and both are removed together).
///
/// Borrows its `Database` and `IImageStore`; both must outlive the service.
class ImageService {
 public:
  ImageService(db::Database& db, IImageStore& store);

  /// Copies `source` into the store and records an `image` row for `coin_id`.
  /// Returns validation errors if the source is missing or not a supported
  /// image type; throws `ImageStoreError`/`db::DatabaseError` on I/O or storage
  /// failure (the copied file is rolled back if the row insert fails).
  /// `original_name` records the source's display name; when omitted it defaults
  /// to `source.filename()`. Callers that copy through a temp file (e.g. an HTTP
  /// upload) should pass the real uploaded filename here.
  [[nodiscard]] std::expected<Image, ValidationErrors> add_image(
      Id coin_id, const std::filesystem::path& source, std::optional<ImageKind> kind,
      std::optional<std::string> caption, std::optional<std::string> original_name = std::nullopt);

  /// Lists the images for a coin (ordered by id).
  [[nodiscard]] std::vector<Image> list_images(Id coin_id);

  /// Fetches a single image row by id, or `std::nullopt` if none exists.
  [[nodiscard]] std::optional<Image> get_image(Id image_id);

  /// Removes one image: deletes its row and its stored file. Returns whether an
  /// image with that id existed.
  [[nodiscard]] bool remove_image(Id image_id);

  /// Removes all image rows and files for a coin. Intended to be called as part
  /// of deleting a coin, before the coin row is removed (the row cascade alone
  /// would leave the files orphaned).
  void purge_coin_images(Id coin_id);

 private:
  db::Database& db_;
  IImageStore& store_;
};

}  // namespace coins

#endif  // COINS_IMAGE_SERVICE_HPP
