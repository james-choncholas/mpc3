#ifndef NET_BENCH_LIB_HPP
#define NET_BENCH_LIB_HPP

#include <mpc3/net_bench_lib_export.hpp>

[[nodiscard]] NET_BENCH_LIB_EXPORT int factorial(int) noexcept;

[[nodiscard]] constexpr int factorial_constexpr(int input) noexcept
{
  if (input == 0) { return 1; }

  return input * factorial_constexpr(input - 1);
}

#endif
