#include <mpc3/net_bench_lib.hpp>

namespace mpc3 {

int factorial(int input) noexcept
{
  int result = 1;

  while (input > 0) {
    result *= input;
    --input;
  }

  return result;
}
}// namespace mpc3
