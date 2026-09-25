# coins_cli: thin command-line interface over coins::core.
#
# The command logic lives in a small static library (coins_cli_lib) so it can be
# driven directly from tests with injected I/O streams; main.cpp is a thin shim.
add_library(coins_cli_lib STATIC
  src/cli_app.cpp)
add_library(coins::cli_lib ALIAS coins_cli_lib)

target_include_directories(coins_cli_lib
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(coins_cli_lib
  PUBLIC
    coins::core
  PRIVATE
    CLI11::CLI11
    coins::warnings)

add_executable(coins_cli
  src/main.cpp)

target_link_libraries(coins_cli
  PRIVATE
    coins::cli_lib
    coins::warnings)
