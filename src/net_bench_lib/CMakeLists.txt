include(GenerateExportHeader)

add_library(net_bench_lib net_bench_lib.cpp)
add_library(mpc3::net_bench_lib ALIAS net_bench_lib)

target_link_libraries(net_bench_lib
  PRIVATE
    mpc3_options
    mpc3_warnings)

# following libs used in hpp file (due to templating)
# so needs public
target_link_system_libraries(
  net_bench_lib
  PUBLIC
    emp-tool 
    transputation
    pthread
    fmt::fmt
    spdlog::spdlog)

# need to add the directory where emp-tool.so is to the rpath
# so linker and ldd can find it.
get_filename_component(EMP_TOOL_LIB_DIR "${EMP-TOOL_LIBRARY}" DIRECTORY)
target_link_directories(net_bench_lib PUBLIC "${EMP_TOOL_LIB_DIR}")

target_include_directories(net_bench_lib 
  ${WARNING_GUARD}
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>)
target_include_directories(net_bench_lib PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")

target_compile_features(net_bench_lib PUBLIC cxx_std_20)

set_target_properties(
  net_bench_lib
  PROPERTIES VERSION ${PROJECT_VERSION}
             CXX_VISIBILITY_PRESET hidden
             VISIBILITY_INLINES_HIDDEN YES)

generate_export_header(net_bench_lib EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/mpc3/net_bench_lib_export.hpp)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(net_bench_lib PUBLIC net_bench_STATIC_DEFINE)
endif()
