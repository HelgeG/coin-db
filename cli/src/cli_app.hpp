#ifndef COINS_CLI_CLI_APP_HPP
#define COINS_CLI_CLI_APP_HPP

#include <iosfwd>

namespace coins::cli {

/// Runs the coins-db CLI. `argv` is the usual argc/argv (argv[0] is the program
/// name). I/O is injected so the app is testable: prompts read from `in`,
/// results are written to `out`, and errors to `err`. Returns a process exit
/// code (0 on success, non-zero on error).
int run(int argc, const char* const* argv, std::istream& in, std::ostream& out, std::ostream& err);

}  // namespace coins::cli

#endif  // COINS_CLI_CLI_APP_HPP
