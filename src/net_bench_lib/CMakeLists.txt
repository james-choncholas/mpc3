add_library(net_bench_lib net_bench_lib.cpp)
add_library(mpc3::net_bench_lib ALIAS net_bench_lib)

target_link_libraries(net_bench_lib PRIVATE mpc3_options mpc3_warnings)

target_include_directories(net_bench_lib ${WARNING_GUARD} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                                                                  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>)

target_compile_features(net_bench_lib PUBLIC cxx_std_20)

set_target_properties(
  net_bench_lib
  PROPERTIES VERSION ${PROJECT_VERSION}
             CXX_VISIBILITY_PRESET hidden
             VISIBILITY_INLINES_HIDDEN YES)

include(GenerateExportHeader)
generate_export_header(net_bench_lib EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/mpc3/net_bench_lib_export.hpp)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(net_bench_lib PUBLIC net_bench_STATIC_DEFINE)
endif()
