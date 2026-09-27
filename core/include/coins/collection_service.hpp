#ifndef COINS_COLLECTION_SERVICE_HPP
#define COINS_COLLECTION_SERVICE_HPP

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "coins/clock.hpp"
#include "coins/coin.hpp"
#include "coins/coin_query.hpp"
#include "coins/collection_io.hpp"
#include "coins/collection_summary.hpp"
#include "coins/currency_unit.hpp"
#include "coins/db/database.hpp"
#include "coins/fs_image_store.hpp"
#include "coins/id.hpp"
#include "coins/image.hpp"
#include "coins/lookup_entry.hpp"
#include "coins/lookup_kind.hpp"
#include "coins/reference_link.hpp"
#include "coins/validation.hpp"
#include "coins/value_estimate.hpp"

namespace coins {

/// Application-facing facade over the data layer. Owns the database connection,
/// the filesystem image store, and a system clock, and wires the repositories
/// and services together so the CLI and REST server share one entry point.
///
/// It is the home for cross-cutting operations — most importantly deleting a
/// coin, which purges its image files and rows before removing the coin so no
/// orphaned files remain.
class CollectionService {
 public:
  /// Opens (creating if needed) the database at `db_path` and roots the image
  /// store at `image_root`. Bootstraps the schema.
  CollectionService(const std::filesystem::path& db_path, std::filesystem::path image_root);

  /// Convenience: a collection under one data directory (`<dir>/coins.db` and
  /// `<dir>/images/`). Creates the directory if needed.
  [[nodiscard]] static CollectionService from_data_dir(const std::filesystem::path& data_dir);

  CollectionService(const CollectionService&) = delete;
  CollectionService& operator=(const CollectionService&) = delete;

  // --- Coins --------------------------------------------------------------
  [[nodiscard]] std::expected<Coin, ValidationErrors> add_coin(const Coin& coin);
  [[nodiscard]] std::optional<Coin> get_coin(Id id);
  [[nodiscard]] std::vector<Coin> search(const CoinQuery& query);
  [[nodiscard]] std::expected<bool, ValidationErrors> update_coin(const Coin& coin);
  /// Deletes a coin, first purging its image files and rows. Returns whether a
  /// coin with that id existed.
  bool delete_coin(Id id);

  // --- Value estimates ----------------------------------------------------
  [[nodiscard]] std::expected<ValueEstimate, ValidationErrors> add_estimate(
      const ValueEstimate& estimate);
  [[nodiscard]] std::vector<ValueEstimate> estimate_history(Id coin_id);
  [[nodiscard]] std::optional<ValueEstimate> latest_estimate(Id coin_id);

  // --- Reference links ----------------------------------------------------
  [[nodiscard]] std::expected<ReferenceLink, ValidationErrors> add_link(const ReferenceLink& link);
  [[nodiscard]] std::vector<ReferenceLink> links(Id coin_id);
  bool remove_link(Id id);

  // --- Images -------------------------------------------------------------
  [[nodiscard]] std::expected<Image, ValidationErrors> add_image(
      Id coin_id, const std::filesystem::path& source, std::optional<ImageKind> kind,
      std::optional<std::string> caption, std::optional<std::string> original_name = std::nullopt);
  [[nodiscard]] std::vector<Image> images(Id coin_id);
  [[nodiscard]] std::optional<Image> get_image(Id image_id);
  bool remove_image(Id image_id);
  /// Absolute path of a stored image (for display/opening).
  [[nodiscard]] std::filesystem::path resolve_image(std::string_view stored_path) const;

  // --- Summary & import/export -------------------------------------------
  [[nodiscard]] CollectionSummary summary(SummaryType type = kDefaultSummaryType,
                                          std::string_view lang = "en");

  // --- Lookups (controlled vocabularies) ---------------------------------
  /// Lists a vocabulary's entries for display in `lang`.
  [[nodiscard]] std::vector<LookupEntry> lookups(LookupKind kind, std::string_view lang);
  [[nodiscard]] std::optional<LookupEntry> lookup(Id id);
  /// Resolves free text (code or localized name) to an entry, creating one when
  /// no match exists (see LookupService::resolve_or_create).
  [[nodiscard]] LookupEntry resolve_lookup(LookupKind kind, std::string_view lang,
                                           std::string_view text);
  /// Currency units: list, fetch, major unit, and resolve/create by code/name.
  [[nodiscard]] std::vector<CurrencyUnit> currency_units(Id currency_id);
  [[nodiscard]] std::optional<CurrencyUnit> currency_unit(Id unit_id);
  [[nodiscard]] std::optional<CurrencyUnit> major_currency_unit(Id currency_id);
  [[nodiscard]] CurrencyUnit resolve_currency_unit(Id currency_id, std::string_view lang,
                                                   std::string_view text);

  // --- Collection settings (base currency) -------------------------------
  /// The collection's base currency (a `currency` lookup entry; defaults to EUR).
  [[nodiscard]] std::optional<LookupEntry> base_currency();
  /// Sets the base currency; returns false if `currency_id` is not a currency.
  [[nodiscard]] bool set_base_currency(Id currency_id);

  [[nodiscard]] std::string export_json();
  [[nodiscard]] std::expected<ImportStats, ValidationErrors> import_json(
      std::string_view json_text);
  /// CSV export with encoded fields rendered as localized names in `lang`.
  [[nodiscard]] std::string export_csv(std::string_view lang = "en");

  /// Current date as an ISO 8601 string (used to default estimate dates).
  [[nodiscard]] std::string today() const;

 private:
  db::Database db_;
  FilesystemImageStore store_;
  SystemClock clock_;
};

}  // namespace coins

#endif  // COINS_COLLECTION_SERVICE_HPP
