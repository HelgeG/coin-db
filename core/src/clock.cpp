#include "coins/clock.hpp"

#include <chrono>
#include <format>

namespace coins {

std::string SystemClock::now_iso8601() const {
  // Truncate to whole seconds for a stable, second-precision ISO 8601 string.
  const auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
  // system_clock's epoch is UTC, so this formats as UTC; the trailing 'Z' marks
  // it explicitly.
  return std::format("{:%Y-%m-%dT%H:%M:%SZ}", now);
}

}  // namespace coins
