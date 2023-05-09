add_executable(stan_bench stan_bench.cpp)

target_link_libraries(
  stan_bench
  PRIVATE 
    transputation
    mpc3::mpc3_options
    mpc3::mpc3_warnings)

target_link_system_libraries(
  stan_bench
  PRIVATE
    pthread
    CLI11::CLI11
    fmt::fmt
    spdlog::spdlog
    lefticus::tools)

target_include_directories(stan_bench PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")
