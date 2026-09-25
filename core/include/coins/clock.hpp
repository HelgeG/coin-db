#ifndef COINS_CLOCK_HPP
#define COINS_CLOCK_HPP

#include <string>

namespace coins {

/// Source of the current time, as an ISO 8601 UTC timestamp string. Abstracted
/// so the data layer's `created_at` / `updated_at` values are deterministic in
/// tests (inject a fixed clock) while using the real wall clock in production.
class IClock {
 public:
  virtual ~IClock() = default;

  /// Current UTC time formatted as ISO 8601, e.g. "2026-01-01T12:34:56Z".
  [[nodiscard]] virtual std::string now_iso8601() const = 0;
};

/// Real-time clock backed by `std::chrono::system_clock`.
class SystemClock final : public IClock {
 public:
  [[nodiscard]] std::string now_iso8601() const override;
};

}  // namespace coins

#endif  // COINS_CLOCK_HPP
