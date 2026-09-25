#ifndef COINS_ID_HPP
#define COINS_ID_HPP

#include <cstdint>

namespace coins {

/// Primary-key type for all persisted entities. Maps to SQLite's INTEGER
/// (a 64-bit signed rowid). The sentinel `kUnsavedId` marks an entity that has
/// not yet been written to the database and therefore has no assigned id.
using Id = std::int64_t;

inline constexpr Id kUnsavedId = 0;

}  // namespace coins

#endif  // COINS_ID_HPP
