# Test suite (GoogleTest). Organized to mirror the source tree under tests/.
#
# Testing philosophy for coins-db: extensive coverage. Every module in
# coins::core should have unit tests; CLI and server get integration tests as
# those components grow. Prefer fast, isolated tests using in-memory/fake
# implementations of the core interfaces (see design.md).

# Helper: register a GoogleTest executable, link it against gtest_main and the
# shared warnings, and auto-discover its test cases with CTest.
function(coins_add_test test_name)
  # Remaining ARGN are source files.
  add_executable(${test_name} ${ARGN})
  target_link_libraries(${test_name}
    PRIVATE
      coins::core
      GTest::gtest
      GTest::gtest_main
      rapidcheck            # property-based testing core
      rapidcheck_gtest      # RC_GTEST_PROP integration with GoogleTest
      coins::warnings)
  # Let test sources include the shared test-support headers as "support/...".
  target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  gtest_discover_tests(${test_name})
endfunction()

# --- core tests -----------------------------------------------------------
coins_add_test(core_version_test core/version_test.cpp)

# Property-based tests (RapidCheck). Demonstrates the harness now; real
# properties (validation, value/summary math, import/export round-trip, etc.)
# are added alongside their modules in later phases.
coins_add_test(core_version_property_test core/version_property_test.cpp)

# Phase 1 — data layer: domain types + SQLite access + schema.
coins_add_test(core_image_kind_test core/image_kind_test.cpp)
coins_add_test(core_database_test core/db/database_test.cpp)
coins_add_test(core_schema_test core/db/schema_test.cpp)

# Phase 2 — coin CRUD & validation.
coins_add_test(core_validation_test core/validation_test.cpp)
coins_add_test(core_sqlite_coin_repository_test core/db/sqlite_coin_repository_test.cpp)

# Phase 3 — references & managed image store.
coins_add_test(core_sqlite_reference_link_repository_test
  core/db/sqlite_reference_link_repository_test.cpp)
coins_add_test(core_fs_image_store_test core/fs_image_store_test.cpp)
coins_add_test(core_image_service_test core/image_service_test.cpp)

# Phase 4 — value tracking.
coins_add_test(core_sqlite_value_estimate_repository_test
  core/db/sqlite_value_estimate_repository_test.cpp)
coins_add_test(core_summary_service_test core/summary_service_test.cpp)

# Phase 5 — search / filter / sort.
coins_add_test(core_sqlite_coin_search_test core/db/sqlite_coin_search_test.cpp)

# Phase 6 — import / export.
coins_add_test(core_collection_io_test core/collection_io_test.cpp)

# Phase 7 — application facade.
coins_add_test(core_collection_service_test core/collection_service_test.cpp)

# Phase 14 — collection base-currency settings.
coins_add_test(core_settings_service_test core/settings_service_test.cpp)

# Phase 7 — CLI. Links the CLI library (which exposes cli_app.hpp) and drives it
# with injected I/O streams. Not via coins_add_test since it needs coins::cli_lib.
add_executable(cli_app_test cli/cli_app_test.cpp)
target_link_libraries(cli_app_test
  PRIVATE
    coins::cli_lib
    GTest::gtest
    GTest::gtest_main
    coins::warnings)
gtest_discover_tests(cli_app_test)

# Phase 8 — REST server integration tests (real httplib client on an ephemeral
# port). Links coins::server_lib and httplib directly.
add_executable(server_app_test server/server_app_test.cpp)
target_link_libraries(server_app_test
  PRIVATE
    coins::server_lib
    httplib::httplib
    GTest::gtest
    GTest::gtest_main
    coins::warnings)
gtest_discover_tests(server_app_test)
