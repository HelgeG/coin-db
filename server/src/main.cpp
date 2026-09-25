#include <httplib.h>

#include <iostream>
#include <string>

#include "coins/collection_service.hpp"
#include "server_app.hpp"

int main(int argc, char** argv) {
  // Minimal args: optional data directory (default "coins-data").
  const std::string data_dir = argc > 1 ? argv[1] : "coins-data";
  constexpr const char* kHost = "127.0.0.1";  // localhost only: no auth in v1
  constexpr int kPort = 8080;

  coins::CollectionService service = coins::CollectionService::from_data_dir(data_dir);

  httplib::Server server;
  coins::server::register_routes(server, service);

  std::cout << "coins-db server on http://" << kHost << ':' << kPort << "  (data dir: " << data_dir
            << ")\n";
  std::cout << "WARNING: single-user, unauthenticated API — bound to localhost only.\n";
  if (!server.listen(kHost, kPort)) {
    std::cerr << "error: could not bind " << kHost << ':' << kPort << "\n";
    return 1;
  }
  return 0;
}
