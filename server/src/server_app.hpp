#ifndef COINS_SERVER_SERVER_APP_HPP
#define COINS_SERVER_SERVER_APP_HPP

namespace httplib {
class Server;
}

namespace coins {
class CollectionService;
}

namespace coins::server {

/// Registers all coins-db REST routes on `server`, backed by `service`. The
/// server and service are borrowed and must outlive the routes. Kept separate
/// from binding/listening so tests can drive routes over an ephemeral port.
///
/// The API is single-user and unauthenticated by design (v1); callers must bind
/// it to localhost only.
void register_routes(httplib::Server& server, CollectionService& service);

}  // namespace coins::server

#endif  // COINS_SERVER_SERVER_APP_HPP
