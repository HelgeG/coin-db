# coins_core: the shared library holding all domain logic. No UI concerns.
add_library(coins_core
  src/version.cpp
  src/image.cpp
  src/grade.cpp
  src/lookup_entry.cpp
  src/lookup_service.cpp
  src/validation.cpp
  src/clock.cpp
  src/fs_image_store.cpp
  src/image_service.cpp
  src/summary_service.cpp
  src/json.cpp
  src/collection_io.cpp
  src/collection_service.cpp
  src/db/database.cpp
  src/db/statement.cpp
  src/db/schema.cpp
  src/db/seed.cpp
  src/db/sqlite_lookup_repository.cpp
  src/db/sqlite_coin_repository.cpp
  src/db/sqlite_reference_link_repository.cpp
  src/db/sqlite_value_estimate_repository.cpp)
add_library(coins::core ALIAS coins_core)

target_include_directories(coins_core
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(coins_core
  PUBLIC
    SQLite::SQLite3
    nlohmann_json::nlohmann_json
  PRIVATE
    coins::warnings)
