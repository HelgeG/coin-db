#ifndef COINS_COIN_HPP
#define COINS_COIN_HPP

#include <optional>
#include <string>

#include "coins/id.hpp"

namespace coins {

/// A single catalogued coin: one row of the `coin` table.
///
/// Only `country` and the year range are required; every other attribute is
/// optional and modelled with `std::optional`. Related data (value estimates,
/// reference links, images) lives in its own types rather than being embedded
/// here, keeping this type a faithful, single-responsibility mapping of the
/// `coin` row. Value estimates and acquisition price are always in EUR; the
/// coin's own face-value currency is `coin_currency` and is never converted.
struct Coin {
  Id id = kUnsavedId;

  // Required.
  std::string country;
  int year_from = 0;
  int year_to = 0;

  // Denomination / face value.
  std::optional<std::string> denomination;  // e.g. "50 Øre"
  std::optional<double> face_value;
  std::optional<std::string> coin_currency;  // ISO 4217, e.g. "NOK"

  // Minting.
  std::optional<std::string> mint;
  std::optional<std::string> mint_mark;

  // Physical attributes.
  std::optional<std::string> composition;  // e.g. "Silver .900"
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
