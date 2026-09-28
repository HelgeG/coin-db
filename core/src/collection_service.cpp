#include "coins/collection_service.hpp"

#include <utility>

#include "coins/db/schema.hpp"
#include "coins/db/sqlite_coin_repository.hpp"
#include "coins/db/sqlite_lookup_repository.hpp"
#include "coins/db/sqlite_reference_link_repository.hpp"
#include "coins/db/sqlite_value_estimate_repository.hpp"
#include "coins/image_service.hpp"
#include "coins/lookup_service.hpp"
#include "coins/settings_service.hpp"
#include "coins/summary_service.hpp"

namespace coins {

CollectionService::CollectionService(const std::filesystem::path& db_path,
                                     std::filesystem::path image_root)
    : db_(db_path.string()), store_(std::move(image_root)) {
  db::bootstrap_schema(db_);
}

CollectionService CollectionService::from_data_dir(const std::filesystem::path& data_dir) {
  std::filesystem::create_directories(data_dir);
  return CollectionService{data_dir / "coins.db", data_dir / "images"};
}

std::expected<Coin, ValidationErrors> CollectionService::add_coin(const Coin& coin) {
  db::SqliteCoinRepository coins{db_, clock_};
  return coins.create(coin);
}

std::optional<Coin> CollectionService::get_coin(Id id) {
  db::SqliteCoinRepository coins{db_, clock_};
  return coins.get(id);
}

std::vector<Coin> CollectionService::search(const CoinQuery& query) {
  db::SqliteCoinRepository coins{db_, clock_};
  return coins.search(query);
}

std::expected<bool, ValidationErrors> CollectionService::update_coin(const Coin& coin) {
  db::SqliteCoinRepository coins{db_, clock_};
  return coins.update(coin);
}

bool CollectionService::delete_coin(Id id) {
  // Purge image files + rows first; the coin's row cascade would otherwise leave
  // the files orphaned.
  ImageService images{db_, store_};
  images.purge_coin_images(id);
  db::SqliteCoinRepository coins{db_, clock_};
  return coins.remove(id);
}

std::expected<ValueEstimate, ValidationErrors> CollectionService::add_estimate(
    const ValueEstimate& estimate) {
  db::SqliteValueEstimateRepository estimates{db_};
  return estimates.add(estimate);
}

std::vector<ValueEstimate> CollectionService::estimate_history(Id coin_id) {
  db::SqliteValueEstimateRepository estimates{db_};
  return estimates.list_for_coin(coin_id);
}

std::optional<ValueEstimate> CollectionService::latest_estimate(Id coin_id) {
  db::SqliteValueEstimateRepository estimates{db_};
  return estimates.latest_for_coin(coin_id);
}

std::expected<ReferenceLink, ValidationErrors> CollectionService::add_link(
    const ReferenceLink& link) {
  db::SqliteReferenceLinkRepository links{db_};
  return links.add(link);
}

std::vector<ReferenceLink> CollectionService::links(Id coin_id) {
  db::SqliteReferenceLinkRepository links{db_};
  return links.list(coin_id);
}

bool CollectionService::remove_link(Id id) {
  db::SqliteReferenceLinkRepository links{db_};
  return links.remove(id);
}

std::expected<Image, ValidationErrors> CollectionService::add_image(
    Id coin_id, const std::filesystem::path& source, std::optional<ImageKind> kind,
    std::optional<std::string> caption, std::optional<std::string> original_name) {
  ImageService images{db_, store_};
  return images.add_image(coin_id, source, kind, std::move(caption), std::move(original_name));
}

std::vector<Image> CollectionService::images(Id coin_id) {
  ImageService images{db_, store_};
  return images.list_images(coin_id);
}

std::optional<Image> CollectionService::get_image(Id image_id) {
  ImageService images{db_, store_};
  return images.get_image(image_id);
}

bool CollectionService::remove_image(Id image_id) {
  ImageService images{db_, store_};
  return images.remove_image(image_id);
}

std::filesystem::path CollectionService::resolve_image(std::string_view stored_path) const {
  return store_.resolve(stored_path);
}

CollectionSummary CollectionService::summary(SummaryType type, std::string_view lang) {
  SummaryService summary{db_};
  return summary.summarize(type, lang);
}

std::vector<LookupEntry> CollectionService::lookups(LookupKind kind, std::string_view lang) {
  db::SqliteLookupRepository repo{db_};
  LookupService service{repo};
  return service.list(kind, lang);
}

std::optional<LookupEntry> CollectionService::lookup(Id id) {
  db::SqliteLookupRepository repo{db_};
  return repo.find_by_id(id);
}

LookupEntry CollectionService::resolve_lookup(LookupKind kind, std::string_view lang,
                                              std::string_view text) {
  db::SqliteLookupRepository repo{db_};
  LookupService service{repo};
  return service.resolve_or_create(kind, lang, text);
}

std::vector<CurrencyUnit> CollectionService::currency_units(Id currency_id) {
  db::SqliteLookupRepository repo{db_};
  return repo.list_units(currency_id);
}

std::optional<CurrencyUnit> CollectionService::currency_unit(Id unit_id) {
  db::SqliteLookupRepository repo{db_};
  return repo.find_unit_by_id(unit_id);
}

std::optional<CurrencyUnit> CollectionService::major_currency_unit(Id currency_id) {
  db::SqliteLookupRepository repo{db_};
  return repo.major_unit(currency_id);
}

CurrencyUnit CollectionService::resolve_currency_unit(Id currency_id, std::string_view lang,
                                                      std::string_view text) {
  db::SqliteLookupRepository repo{db_};
  LookupService service{repo};
  return service.resolve_or_create_unit(currency_id, lang, text);
}

std::optional<LookupEntry> CollectionService::base_currency() {
  db::SqliteLookupRepository repo{db_};
  SettingsService settings{db_, repo};
  return settings.base_currency();
}

bool CollectionService::set_base_currency(Id currency_id) {
  db::SqliteLookupRepository repo{db_};
  SettingsService settings{db_, repo};
  return settings.set_base_currency(currency_id);
}

std::string CollectionService::export_json() { return coins::export_json(db_); }

std::expected<ImportStats, ValidationErrors> CollectionService::import_json(
    std::string_view json_text) {
  return coins::import_json(db_, json_text);
}

std::string CollectionService::export_csv(std::string_view lang) {
  return coins::export_csv(db_, lang);
}

std::string CollectionService::export_csv(const CoinQuery& query) {
  db::SqliteCoinRepository coins{db_, clock_};
  const std::vector<Coin> matches = coins.search(query);
  std::vector<Id> ids;
  ids.reserve(matches.size());
  for (const Coin& coin : matches) {
    ids.push_back(coin.id);
  }
  return coins::export_csv(db_, ids, query.lang);
}

std::string CollectionService::today() const {
  const std::string now = clock_.now_iso8601();
  return now.substr(0, 10);  // YYYY-MM-DD
}

}  // namespace coins
