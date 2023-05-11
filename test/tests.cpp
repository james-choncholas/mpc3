#include <catch2/catch_test_macros.hpp>

#include <mpc3/net_bench_lib.hpp>


TEST_CASE("Factorials are computed", "[factorial]")
{
  REQUIRE(mpc3::factorial(0) == 1);
  REQUIRE(mpc3::factorial(1) == 1);
  REQUIRE(mpc3::factorial(2) == 2);
  REQUIRE(mpc3::factorial(3) == 6);
  REQUIRE(mpc3::factorial(10) == 3628800);
}
