#ifndef COINS_VALIDATION_HPP
#define COINS_VALIDATION_HPP

#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "coins/coin.hpp"
#include "coins/reference_link.hpp"
#include "coins/value_estimate.hpp"

namespace coins {

/// A single domain-validation failure: which field was rejected and why.
struct ValidationError {
  std::string field;
  std::string message;

  friend bool operator==(const ValidationError&, const ValidationError&) = default;
};

/// The set of validation failures for one entity (empty means valid).
using ValidationErrors = std::vector<ValidationError>;

/// Result of a validating operation: either success (`void`) or the list of
/// failures. This is the project's convention for *expected* failures, as
/// opposed to exceptions for exceptional/infrastructure errors (see design.md).
using ValidationResult = std::expected<void, ValidationErrors>;

/// Validates a coin against the domain rules (design.md, "Validation Rules"):
///   - `country` is required (non-empty after trimming whitespace);
///   - a real year is required and the range is ordered (1 <= year_from <= year_to);
///   - `coin_currency`, when present, is a 3-letter uppercase ISO 4217 code;
///   - monetary/measurement values, when present, are non-negative;
///   - grade is validated per scale: a Sheldon `grade_numeric` is within 1..70,
///     a Norwegian grade carries a valid symbolic `grade_label`, and any other
///     scale is accepted as-is.
/// Returns all failures found, not just the first.
[[nodiscard]] ValidationResult validate_coin(const Coin& coin);

/// True if `url` is a syntactically valid http/https URL with a non-empty host.
/// This is a shape check, not a reachability check.
[[nodiscard]] bool is_valid_url(std::string_view url);

/// Validates a reference link: `label` is required (non-blank) and `url` must be
/// a valid http(s) URL.
[[nodiscard]] ValidationResult validate_reference_link(const ReferenceLink& link);

/// Validates a value estimate: `amount_eur` must be non-negative and
/// `estimated_at` must be an ISO 8601 date (YYYY-MM-DD).
[[nodiscard]] ValidationResult validate_value_estimate(const ValueEstimate& estimate);

}  // namespace coins

#endif  // COINS_VALIDATION_HPP
