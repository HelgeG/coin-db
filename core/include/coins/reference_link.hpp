#ifndef COINS_REFERENCE_LINK_HPP
#define COINS_REFERENCE_LINK_HPP

#include <string>

#include "coins/id.hpp"

namespace coins {

/// An external reference link attached to a coin: one row of the
/// `reference_link` table (e.g. a Numista or PCGS entry, or an auction listing).
struct ReferenceLink {
  Id id = kUnsavedId;
  Id coin_id = kUnsavedId;

  std::string label;  // e.g. "Numista"
  std::string url;
};

}  // namespace coins

#endif  // COINS_REFERENCE_LINK_HPP
