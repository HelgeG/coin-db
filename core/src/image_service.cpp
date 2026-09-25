#include "coins/image_service.hpp"

#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "coins/db/database.hpp"
#include "coins/db/statement.hpp"

namespace coins {

using db::Statement;

ImageService::ImageService(db::Database& db, IImageStore& store) : db_(db), store_(store) {}

std::expected<Image, ValidationErrors> ImageService::add_image(
    Id coin_id, const std::filesystem::path& source, std::optional<ImageKind> kind,
    std::optional<std::string> caption, std::optional<std::string> original_name) {
  // Expected, user-facing preconditions are reported as validation errors.
  std::error_code ec;
  if (!std::filesystem::is_regular_file(source, ec) || ec) {
    return std::unexpected(ValidationErrors{{"source", "file does not exist"}});
  }
  if (!is_supported_image_extension(source.extension().string())) {
    return std::unexpected(ValidationErrors{{"source", "unsupported image type"}});
  }

  // Copy the bytes in first, then record the row. If the insert fails, roll the
  // file back so we never leave an orphaned file behind.
  const std::optional<std::string> name =
      original_name.has_value() ? original_name
                                : std::optional<std::string>{source.filename().string()};

  const std::string stored_path = store_.store(source, coin_id);
  try {
    Statement stmt = db_.prepare(
        "INSERT INTO image (coin_id, kind, stored_path, original_name, caption) "
        "VALUES (?, ?, ?, ?, ?);");
    stmt.bind(1, coin_id);
    const std::optional<std::string> kind_text =
        kind.has_value() ? std::optional<std::string>{std::string{to_string(*kind)}} : std::nullopt;
    stmt.bind(2, kind_text);
    stmt.bind(3, std::string_view{stored_path});
    stmt.bind(4, name);
    stmt.bind(5, caption);
    (void)stmt.step();

    Image image;
    image.id = db_.last_insert_rowid();
    image.coin_id = coin_id;
    image.kind = kind;
    image.stored_path = stored_path;
    image.original_name = name;
    image.caption = std::move(caption);
    return image;
  } catch (...) {
    store_.remove(stored_path);
    throw;
  }
}

std::vector<Image> ImageService::list_images(Id coin_id) {
  std::vector<Image> images;
  Statement stmt = db_.prepare(
      "SELECT id, coin_id, kind, stored_path, original_name, caption FROM image "
      "WHERE coin_id = ? ORDER BY id;");
  stmt.bind(1, coin_id);
  while (stmt.step()) {
    Image image;
    image.id = stmt.column_int64(0);
    image.coin_id = stmt.column_int64(1);
    if (const std::optional<std::string> kind_text = stmt.column_opt_text(2); kind_text) {
      image.kind = image_kind_from_string(*kind_text);
    }
    image.stored_path = stmt.column_text(3);
    image.original_name = stmt.column_opt_text(4);
    image.caption = stmt.column_opt_text(5);
    images.push_back(std::move(image));
  }
  return images;
}

std::optional<Image> ImageService::get_image(Id image_id) {
  Statement stmt = db_.prepare(
      "SELECT id, coin_id, kind, stored_path, original_name, caption FROM image "
      "WHERE id = ?;");
  stmt.bind(1, image_id);
  if (!stmt.step()) {
    return std::nullopt;
  }
  Image image;
  image.id = stmt.column_int64(0);
  image.coin_id = stmt.column_int64(1);
  if (const std::optional<std::string> kind_text = stmt.column_opt_text(2); kind_text) {
    image.kind = image_kind_from_string(*kind_text);
  }
  image.stored_path = stmt.column_text(3);
  image.original_name = stmt.column_opt_text(4);
  image.caption = stmt.column_opt_text(5);
  return image;
}

bool ImageService::remove_image(Id image_id) {
  std::string stored_path;
  {
    Statement stmt = db_.prepare("SELECT stored_path FROM image WHERE id = ?;");
    stmt.bind(1, image_id);
    if (!stmt.step()) {
      return false;
    }
    stored_path = stmt.column_text(0);
  }

  Statement del = db_.prepare("DELETE FROM image WHERE id = ?;");
  del.bind(1, image_id);
  (void)del.step();

  store_.remove(stored_path);
  return true;
}

void ImageService::purge_coin_images(Id coin_id) {
  std::vector<std::string> stored_paths;
  {
    Statement stmt = db_.prepare("SELECT stored_path FROM image WHERE coin_id = ?;");
    stmt.bind(1, coin_id);
    while (stmt.step()) {
      stored_paths.push_back(stmt.column_text(0));
    }
  }

  Statement del = db_.prepare("DELETE FROM image WHERE coin_id = ?;");
  del.bind(1, coin_id);
  (void)del.step();

  for (const std::string& path : stored_paths) {
    store_.remove(path);
  }
}

}  // namespace coins
