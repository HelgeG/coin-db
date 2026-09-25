#ifndef COINS_GRADE_HPP
#define COINS_GRADE_HPP

#include <span>
#include <string_view>

namespace coins {

/// Canonical names for the grading scales the core validates specifically.
/// `grade_scale` on a coin is free text so any scale can be recorded; these
/// names simply drive scale-specific validation. Scales not listed here are
/// accepted as-is (Open/Closed: adding a new controlled scale is additive).
namespace grade_scales {
inline constexpr std::string_view kSheldon = "Sheldon";      // numeric 1..70
inline constexpr std::string_view kNorwegian = "Norwegian";  // symbolic labels
}  // namespace grade_scales

/// The Norwegian grade tokens, ordered from best (index 0) to worst. The grade
/// is symbolic (e.g. "0/01", "1+"), so it is stored in `grade_label`, not
/// `grade_numeric`.
[[nodiscard]] std::span<const std::string_view> norwegian_grades() noexcept;

/// True if `label` is one of the recognized Norwegian grade tokens.
[[nodiscard]] bool is_valid_norwegian_grade(std::string_view label) noexcept;

/// Inclusive bounds of the Sheldon numeric scale.
inline constexpr int kSheldonMin = 1;
inline constexpr int kSheldonMax = 70;

}  // namespace coins

#endif  // COINS_GRADE_HPP
