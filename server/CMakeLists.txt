# coins_server: thin REST/HTTP layer over coins::core.
#
# Route registration lives in a small static library (coins_server_lib) so it can
# be driven from integration tests over an ephemeral port; main.cpp is a shim
# that opens the collection, binds to localhost, and listens.
add_library(coins_server_lib STATIC
  src/server_app.cpp)
add_library(coins::server_lib ALIAS coins_server_lib)

target_include_directories(coins_server_lib
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(coins_server_lib
  PUBLIC
    coins::core
  PRIVATE
    httplib::httplib
    coins::warnings)

add_executable(coins_server
  src/main.cpp)

target_link_libraries(coins_server
  PRIVATE
    coins::server_lib
    httplib::httplib
    coins::warnings)
