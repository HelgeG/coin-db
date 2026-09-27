#include "coins/json.hpp"

#include <optional>
#include <string>

namespace coins {
namespace {

using json = nlohmann::json;

json opt_to_json(const std::optional<std::string>& value) {
  return value.has_value() ? json(*value) : json(nullptr);
}
json opt_to_json(const std::optional<double>& value) {
  return value.has_value() ? json(*value) : json(nullptr);
}
json opt_to_json(const std::optional<int>& value) {
  return value.has_value() ? json(*value) : json(nullptr);
}
json opt_to_json(const std::optional<Id>& value) {
  return value.has_value() ? json(*value) : json(nullptr);
}

std::optional<std::string> opt_string(const json& obj, const char* key) {
  if (!obj.contains(key) || obj.at(key).is_null()) {
    return std::nullopt;
  }
  return obj.at(key).get<std::string>();
}
std::optional<double> opt_double(const json& obj, const char* key) {
  if (!obj.contains(key) || obj.at(key).is_null()) {
    return std::nullopt;
  }
  return obj.at(key).get<double>();
}
std::optional<int> opt_int(const json& obj, const char* key) {
  if (!obj.contains(key) || obj.at(key).is_null()) {
    return std::nullopt;
  }
  return obj.at(key).get<int>();
}
std::optional<Id> opt_id(const json& obj, const char* key) {
  if (!obj.contains(key) || obj.at(key).is_null()) {
    return std::nullopt;
  }
  return obj.at(key).get<Id>();
}
std::string req_string(const json& obj, const char* key) {
  return opt_string(obj, key).value_or(std::string{});
}
int req_int(const json& obj, const char* key) { return opt_int(obj, key).value_or(0); }
double req_double(const json& obj, const char* key) { return opt_double(obj, key).value_or(0.0); }
Id req_id(const json& obj, const char* key) {
  if (!obj.contains(key) || obj.at(key).is_null()) {
    return kUnsavedId;
  }
  return obj.at(key).get<Id>();
}

}  // namespace

json coin_to_json(const Coin& coin) {
  return json{{"id", coin.id},
              {"country_id", coin.country_id},
              {"denomination_id", opt_to_json(coin.denomination_id)},
              {"face_value", opt_to_json(coin.face_value)},
              {"currency_id", opt_to_json(coin.currency_id)},
              {"face_unit_id", opt_to_json(coin.face_unit_id)},
              {"year_from", coin.year_from},
              {"year_to", coin.year_to},
              {"mint_id", opt_to_json(coin.mint_id)},
              {"mint_mark", opt_to_json(coin.mint_mark)},
              {"composition_id", opt_to_json(coin.composition_id)},
              {"weight_g", opt_to_json(coin.weight_g)},
              {"diameter_mm", opt_to_json(coin.diameter_mm)},
              {"grade_scale", opt_to_json(coin.grade_scale)},
              {"grade_numeric", opt_to_json(coin.grade_numeric)},
              {"grade_label", opt_to_json(coin.grade_label)},
              {"acquired_date", opt_to_json(coin.acquired_date)},
              {"acquired_price_eur", opt_to_json(coin.acquired_price_eur)},
              {"acquired_source", opt_to_json(coin.acquired_source)},
              {"notes", opt_to_json(coin.notes)},
              {"created_at", coin.created_at},
              {"updated_at", coin.updated_at}};
}

json value_estimate_to_json(const ValueEstimate& estimate) {
  return json{{"id", estimate.id},
              {"coin_id", estimate.coin_id},
              {"amount_eur", estimate.amount_eur},
              {"estimated_at", estimate.estimated_at},
              {"source", opt_to_json(estimate.source)}};
}

json reference_link_to_json(const ReferenceLink& link) {
  return json{{"id", link.id}, {"coin_id", link.coin_id}, {"label", link.label}, {"url", link.url}};
}

json image_to_json(const Image& image) {
  const json kind =
      image.kind.has_value() ? json(std::string{to_string(*image.kind)}) : json(nullptr);
  return json{{"id", image.id},
              {"coin_id", image.coin_id},
              {"kind", kind},
              {"stored_path", image.stored_path},
              {"original_name", opt_to_json(image.original_name)},
              {"caption", opt_to_json(image.caption)}};
}

Coin coin_from_json(const json& obj) {
  Coin coin;
  coin.id = req_id(obj, "id");
  coin.country_id = req_id(obj, "country_id");
  coin.denomination_id = opt_id(obj, "denomination_id");
  coin.face_value = opt_double(obj, "face_value");
  coin.currency_id = opt_id(obj, "currency_id");
  coin.face_unit_id = opt_id(obj, "face_unit_id");
  coin.year_from = req_int(obj, "year_from");
  coin.year_to = req_int(obj, "year_to");
  coin.mint_id = opt_id(obj, "mint_id");
  coin.mint_mark = opt_string(obj, "mint_mark");
  coin.composition_id = opt_id(obj, "composition_id");
  coin.weight_g = opt_double(obj, "weight_g");
  coin.diameter_mm = opt_double(obj, "diameter_mm");
  coin.grade_scale = opt_string(obj, "grade_scale");
  coin.grade_numeric = opt_int(obj, "grade_numeric");
  coin.grade_label = opt_string(obj, "grade_label");
  coin.acquired_date = opt_string(obj, "acquired_date");
  coin.acquired_price_eur = opt_double(obj, "acquired_price_eur");
  coin.acquired_source = opt_string(obj, "acquired_source");
  coin.notes = opt_string(obj, "notes");
  coin.created_at = req_string(obj, "created_at");
  coin.updated_at = req_string(obj, "updated_at");
  return coin;
}

ValueEstimate value_estimate_from_json(const json& obj) {
  ValueEstimate estimate;
  estimate.id = req_id(obj, "id");
  estimate.coin_id = req_id(obj, "coin_id");
  estimate.amount_eur = req_double(obj, "amount_eur");
  estimate.estimated_at = req_string(obj, "estimated_at");
  estimate.source = opt_string(obj, "source");
  return estimate;
}

ReferenceLink reference_link_from_json(const json& obj) {
  ReferenceLink link;
  link.id = req_id(obj, "id");
  link.coin_id = req_id(obj, "coin_id");
  link.label = req_string(obj, "label");
  link.url = req_string(obj, "url");
  return link;
}

Image image_from_json(const json& obj) {
  Image image;
  image.id = req_id(obj, "id");
  image.coin_id = req_id(obj, "coin_id");
  if (const std::optional<std::string> kind = opt_string(obj, "kind"); kind.has_value()) {
    image.kind = image_kind_from_string(*kind);
  }
  image.stored_path = req_string(obj, "stored_path");
  image.original_name = opt_string(obj, "original_name");
  image.caption = opt_string(obj, "caption");
  return image;
}

}  // namespace coins
