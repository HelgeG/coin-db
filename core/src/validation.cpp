#include "coins/validation.hpp"

#include <algorithm>
#include <cctype>
#include <string_view>

#include "coins/grade.hpp"

namespace coins {
namespace {

bool is_blank(std::string_view text) {
  return std::all_of(text.begin(), text.end(),
                     [](unsigned char ch) { return std::isspace(ch) != 0; });
}

void require_non_negative(ValidationErrors& errors, const std::optional<double>& value,
                          std::string field) {
  if (value.has_value() && *value < 0.0) {
    errors.push_back({std::move(field), "must not be negative"});
  }
}

// A minimal ISO 8601 calendar date check: exactly "YYYY-MM-DD" with digits and
// dashes in the right places. Shape only, not calendar validity.
bool is_iso_date(std::string_view text) {
  if (text.size() != 10) {
    return false;
  }
  if (text[4] != '-' || text[7] != '-') {
    return false;
  }
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (i == 4 || i == 7) {
      continue;
    }
    if (text[i] < '0' || text[i] > '9') {
      return false;
    }
  }
  return true;
}

}  // namespace

ValidationResult validate_coin(const Coin& coin) {
  ValidationErrors errors;

  if (coin.country_id <= 0) {
    errors.push_back({"country", "is required"});
  }

  if (coin.year_from < 1) {
    errors.push_back({"year_from", "a valid year is required (>= 1)"});
  }
  if (coin.year_to < coin.year_from) {
    errors.push_back({"year_to", "must be greater than or equal to year_from"});
  }

  require_non_negative(errors, coin.face_value, "face_value");
  require_non_negative(errors, coin.weight_g, "weight_g");
  require_non_negative(errors, coin.diameter_mm, "diameter_mm");
  require_non_negative(errors, coin.acquired_price_eur, "acquired_price_eur");

  // Grade validation is scale-aware. `grade_scale` is free text so any system
  // can be recorded; only the scales the core knows about are constrained, and
  // unknown scales (e.g. adjectival F/VF/XF systems) are accepted as-is.
  if (coin.grade_scale.has_value()) {
    const std::string& scale = *coin.grade_scale;
    if (scale == grade_scales::kSheldon) {
      // Sheldon is numeric; the label (e.g. "MS", "VF") is a free-text adjunct.
      if (coin.grade_numeric.has_value()) {
        const int grade = *coin.grade_numeric;
        if (grade < kSheldonMin || grade > kSheldonMax) {
          errors.push_back({"grade_numeric", "Sheldon grade must be within 1..70"});
        }
      }
    } else if (scale == grade_scales::kNorwegian) {
      // Norwegian grades are symbolic and live in the label, not the number.
      if (!coin.grade_label.has_value()) {
        errors.push_back({"grade_label", "Norwegian grade requires a grade label (e.g. \"1+\")"});
      } else if (!is_valid_norwegian_grade(*coin.grade_label)) {
        errors.push_back(
            {"grade_label", "not a valid Norwegian grade (0, 0/01, 01, 1+, 1, 1-, 2, 3)"});
      }
    }
  }

  if (!errors.empty()) {
    return std::unexpected(std::move(errors));
  }
  return {};
}

bool is_valid_url(std::string_view url) {
  constexpr std::string_view kHttp = "http://";
  constexpr std::string_view kHttps = "https://";

  std::string_view rest;
  if (url.starts_with(kHttps)) {
    rest = url.substr(kHttps.size());
  } else if (url.starts_with(kHttp)) {
    rest = url.substr(kHttp.size());
  } else {
    return false;
  }

  // The authority/host runs up to the first path, query, or fragment delimiter.
  const std::size_t host_end = rest.find_first_of("/?#");
  const std::string_view host =
      host_end == std::string_view::npos ? rest : rest.substr(0, host_end);
  if (host.empty()) {
    return false;
  }
  // Reject obvious junk: whitespace anywhere in the host.
  return std::none_of(host.begin(), host.end(),
                      [](unsigned char ch) { return std::isspace(ch) != 0; });
}

ValidationResult validate_reference_link(const ReferenceLink& link) {
  ValidationErrors errors;

  if (link.label.empty() || is_blank(link.label)) {
    errors.push_back({"label", "is required"});
  }
  if (!is_valid_url(link.url)) {
    errors.push_back({"url", "must be a valid http(s) URL"});
  }

  if (!errors.empty()) {
    return std::unexpected(std::move(errors));
  }
  return {};
}

ValidationResult validate_value_estimate(const ValueEstimate& estimate) {
  ValidationErrors errors;

  if (estimate.amount_eur < 0.0) {
    errors.push_back({"amount_eur", "must not be negative"});
  }
  if (!is_iso_date(estimate.estimated_at)) {
    errors.push_back({"estimated_at", "must be an ISO 8601 date (YYYY-MM-DD)"});
  }

  if (!errors.empty()) {
    return std::unexpected(std::move(errors));
  }
  return {};
}

}  // namespace coins
