#include <array>
#include <optional>
#include <string_view>

#include "coins/image.hpp"
#include "support/rc_gtest.hpp"

namespace {

using coins::ImageKind;

TEST(ImageKindTest, ToStringUsesCanonicalTokens) {
  EXPECT_EQ(coins::to_string(ImageKind::Obverse), "obverse");
  EXPECT_EQ(coins::to_string(ImageKind::Reverse), "reverse");
  EXPECT_EQ(coins::to_string(ImageKind::Detail), "detail");
}

TEST(ImageKindTest, FromStringParsesKnownTokens) {
  EXPECT_EQ(coins::image_kind_from_string("obverse"), ImageKind::Obverse);
  EXPECT_EQ(coins::image_kind_from_string("reverse"), ImageKind::Reverse);
  EXPECT_EQ(coins::image_kind_from_string("detail"), ImageKind::Detail);
}

TEST(ImageKindTest, FromStringRejectsUnknownToken) {
  EXPECT_EQ(coins::image_kind_from_string("edge"), std::nullopt);
  EXPECT_EQ(coins::image_kind_from_string(""), std::nullopt);
  EXPECT_EQ(coins::image_kind_from_string("Obverse"), std::nullopt);  // case-sensitive
}

// Property: to_string / from_string are inverses for every enum value. This is
// the round-trip property shape, and the first RapidCheck property over real
// coins_core domain code.
RC_GTEST_PROP(ImageKindProperty, RoundTripsThroughString, ()) {
  constexpr std::array<ImageKind, 3> kAll{ImageKind::Obverse, ImageKind::Reverse,
                                          ImageKind::Detail};
  const ImageKind kind = *rc::gen::elementOf(kAll);
  RC_ASSERT(coins::image_kind_from_string(coins::to_string(kind)) == kind);
}

}  // namespace
