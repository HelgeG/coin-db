#ifndef COINS_I_REFERENCE_LINK_REPOSITORY_HPP
#define COINS_I_REFERENCE_LINK_REPOSITORY_HPP

#include <expected>
#include <vector>

#include "coins/id.hpp"
#include "coins/reference_link.hpp"
#include "coins/validation.hpp"

namespace coins {

/// Persistence boundary for reference links attached to coins. Same failure
/// conventions as `ICoinRepository`: validation failures via `std::expected`,
/// absence via `bool`, storage failures via exceptions.
class IReferenceLinkRepository {
 public:
  virtual ~IReferenceLinkRepository() = default;

  /// Validates and inserts a link for `link.coin_id`. Returns the stored link
  /// with its assigned id.
  [[nodiscard]] virtual std::expected<ReferenceLink, ValidationErrors> add(
      const ReferenceLink& link) = 0;

  /// Lists the links for a coin (ordered by id).
  [[nodiscard]] virtual std::vector<ReferenceLink> list(Id coin_id) = 0;

  /// Removes a link by id. Returns whether a link was removed.
  [[nodiscard]] virtual bool remove(Id id) = 0;
};

}  // namespace coins

#endif  // COINS_I_REFERENCE_LINK_REPOSITORY_HPP
