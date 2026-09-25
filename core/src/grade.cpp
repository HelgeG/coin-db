#include "coins/grade.hpp"

#include <algorithm>
#include <array>

namespace coins {
namespace {

// Norwegian coin grades, best to worst:
//   0     Uirkulert / nypreget (uncirculated / mint state)
//   0/01  essentially uncirculated with a trace of handling
//   01    meget lett slitt (very lightly worn)
//   1+    lett slitt (lightly worn; ~EF)
//   1     alminnelig slitt (normally worn; ~F)
//   1-    sterkt slitt (heavily worn)
//   2     meget sterkt slitt (very heavily worn / about good)
//   3     dårlig (poor)
constexpr std::array<std::string_view, 8> kNorwegianGrades{"0", "0/01", "01", "1+",
                                                           "1", "1-",   "2",  "3"};

}  // namespace

std::span<const std::string_view> norwegian_grades() noexcept { return kNorwegianGrades; }

bool is_valid_norwegian_grade(std::string_view label) noexcept {
  return std::find(kNorwegianGrades.begin(), kNorwegianGrades.end(), label) !=
         kNorwegianGrades.end();
}

}  // namespace coins
