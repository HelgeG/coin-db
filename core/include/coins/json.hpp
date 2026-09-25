#ifndef COINS_JSON_HPP
#define COINS_JSON_HPP

#include <nlohmann/json.hpp>

#include "coins/coin.hpp"
#include "coins/image.hpp"
#include "coins/reference_link.hpp"
#include "coins/value_estimate.hpp"

namespace coins {

// Entity <-> JSON mapping shared by the import/export layer and the REST server.
// Absent optionals serialize as JSON null; on read, missing or null fields
// become empty optionals. Ids are included on write and read when present, so
// callers (e.g. a REST POST) can omit them and set them from context.

[[nodiscard]] nlohmann::json coin_to_json(const Coin& coin);
[[nodiscard]] nlohmann::json value_estimate_to_json(const ValueEstimate& estimate);
[[nodiscard]] nlohmann::json reference_link_to_json(const ReferenceLink& link);
[[nodiscard]] nlohmann::json image_to_json(const Image& image);

[[nodiscard]] Coin coin_from_json(const nlohmann::json& obj);
[[nodiscard]] ValueEstimate value_estimate_from_json(const nlohmann::json& obj);
[[nodiscard]] ReferenceLink reference_link_from_json(const nlohmann::json& obj);
[[nodiscard]] Image image_from_json(const nlohmann::json& obj);

}  // namespace coins

#endif  // COINS_JSON_HPP
