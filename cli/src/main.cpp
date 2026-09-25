#include <iostream>

#include "cli_app.hpp"

int main(int argc, char** argv) {
  return coins::cli::run(argc, argv, std::cin, std::cout, std::cerr);
}
