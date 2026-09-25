#include "coins/version.hpp"

#include <gtest/gtest.h>

#include <string_view>

namespace {

TEST(VersionTest, ReturnsNonEmptyString) { EXPECT_FALSE(coins::version().empty()); }

TEST(VersionTest, MatchesExpectedValue) { EXPECT_EQ(coins::version(), std::string_view{"0.1.0"}); }

}  // namespace
