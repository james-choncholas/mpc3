add_executable(net_bench_app net_bench_app.cpp)

target_link_libraries(
  net_bench_app
  PRIVATE 
    net_bench_lib
    transputation
    mpc3::mpc3_options
    mpc3::mpc3_warnings)

target_link_system_libraries(
  net_bench_app
  PRIVATE
    pthread
    CLI11::CLI11
    fmt::fmt
    spdlog::spdlog
    lefticus::tools)

target_include_directories(net_bench_app PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")
