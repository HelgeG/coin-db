// Property-based tests for coins_core, using RapidCheck's GoogleTest
// integration (RC_GTEST_PROP). This file currently proves the property-based
// testing harness is wired end-to-end; substantive properties (coin validation
// invariants, value/summary math, import/export round-trips, search/filter
// laws) arrive with their modules in later phases.

#include "coins/version.hpp"

// Pulls in GoogleTest + RapidCheck's gtest integration (see the shim for the
// C++23 compatibility workarounds it applies).
#include <string>
#include <string_view>

#include "support/rc_gtest.hpp"

namespace {

// Property: coins::version() is pure and stable. For any number of successive
// calls, every call returns the identical, non-empty value. RapidCheck
// generates the call count many times over and would shrink it to the minimal
// failing case if the invariant ever broke.
RC_GTEST_PROP(VersionProperty, IsStableAndNonEmptyAcrossCalls, ()) {
  const int call_count = *rc::gen::inRange(1, 1000);

  const std::string_view first = coins::version();
  RC_ASSERT(!first.empty());

  for (int i = 0; i < call_count; ++i) {
    RC_ASSERT(coins::version() == first);
  }
}

// Property: the version string survives a round-trip through std::string
// unchanged. A trivial invariant, included to show a second property in the
// same suite alongside the generated-input one above.
RC_GTEST_PROP(VersionProperty, RoundTripsThroughStdString, ()) {
  const std::string owned{coins::version()};
  RC_ASSERT(std::string_view{owned} == coins::version());
}

}  // namespace
