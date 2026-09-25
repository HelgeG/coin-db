#ifndef COINS_VERSION_HPP
#define COINS_VERSION_HPP

#include <string_view>

namespace coins {

/// Returns the semantic version string of the coins-db core library.
[[nodiscard]] std::string_view version() noexcept;

}  // namespace coins

#endif  // COINS_VERSION_HPP
