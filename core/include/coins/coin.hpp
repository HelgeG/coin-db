#ifndef COINS_COIN_HPP
#define COINS_COIN_HPP

#include <optional>
#include <string>

#include "coins/id.hpp"

namespace coins {

/// A single catalogued coin: one row of the `coin` table.
///
/// Only `country_id` and the year range are required; every other attribute is
/// optional. The encoded fields — country, denomination, mint, composition, and
/// currency — reference shared, localized `lookup_entry` rows by id (see
/// `LookupEntry`); `mint_mark` stays free text. `face_value` is expressed in the
/// optional `face_unit_id` currency unit (the currency's major unit when unset).
/// Related data (value estimates, reference links, images) lives in its own
/// types. Value estimates and acquisition price are always in EUR; the coin's
/// own face-value currency is never converted.
struct Coin {
  Id id = kUnsavedId;

  // Required.
  Id country_id = kUnsavedId;  // lookup_entry, kind = country
  int year_from = 0;
  int year_to = 0;

  // Denomination / face value.
  std::optional<Id> denomination_id;  // lookup_entry, kind = denomination (named piece)
  std::optional<double> face_value;
  std::optional<Id> currency_id;   // lookup_entry, kind = currency
  std::optional<Id> face_unit_id;  // currency_unit; null = currency's major unit

  // Minting.
  std::optional<Id> mint_id;  // lookup_entry, kind = mint
  std::optional<std::string> mint_mark;

  // Physical attributes.
  std::optional<Id> composition_id;  // lookup_entry, kind = composition
  std::optional<double> weight_g;
  std::optional<double> diameter_mm;

  // Condition / grade.
  std::optional<std::string> grade_scale;  // e.g. "Sheldon"
  std::optional<int> grade_numeric;        // e.g. 65
  std::optional<std::string> grade_label;  // e.g. "MS", "VF"

  // Acquisition (price normalized to EUR).
  std::optional<std::string> acquired_date;  // ISO 8601
  std::optional<double> acquired_price_eur;
  std::optional<std::string> acquired_source;

  std::optional<std::string> notes;

  // Timestamps (ISO 8601). Set by the data layer on write.
  std::string created_at;
  std::string updated_at;

  friend bool operator==(const Coin&, const Coin&) = default;
};

}  // namespace coins

#endif  // COINS_COIN_HPP
