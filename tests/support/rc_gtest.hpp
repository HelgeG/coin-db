#ifndef COINS_TESTS_SUPPORT_RC_GTEST_HPP
#define COINS_TESTS_SUPPORT_RC_GTEST_HPP

// Shared include shim for RapidCheck's GoogleTest integration.
//
// RapidCheck (ConanCenter cci.20231215) predates C++23 and does not compile
// cleanly against a C++23 libc++ on its own:
//   * <rapidcheck/Gen.hpp> calls std::current_exception(), but relies on
//     <exception> having been included transitively. Under C++23 libc++ it is
//     not, so `std::exception_ptr` is incomplete and it fails to compile.
//     Including <exception> first makes the type complete.
//   * <rapidcheck/Maybe.h> uses std::aligned_storage, which is deprecated in
//     C++23 and would trip our -Werror build. We silence that one deprecation
//     only across RapidCheck's own headers.
//
// Test files include this header instead of <rapidcheck/gtest.h> so the project
// stays on C++23 with warnings-as-errors intact for our own code. Remove this
// shim if a RapidCheck version with native C++23 support becomes available.

#include <gtest/gtest.h>

#include <exception>  // complete std::exception_ptr for <rapidcheck/Gen.hpp>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <rapidcheck/gtest.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif  // COINS_TESTS_SUPPORT_RC_GTEST_HPP
