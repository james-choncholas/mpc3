cmake_minimum_required(VERSION 3.21)

# This template attempts to be "fetch_content"-able
# so that it works well with tools like CPM or other
# manual dependency management

# Only set the cxx_standard if it is not set by someone else
if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

# strongly encouraged to enable this globally to avoid conflicts between
# -Wpedantic being enabled and -std=c++20 and -std=gnu++20 for example
# when compiling with PCH enabled
set(CMAKE_CXX_EXTENSIONS OFF)

# Set the project name and language
project(
  mpc3
  VERSION 0.0.1
  DESCRIPTION ""
  HOMEPAGE_URL "https://github.com/james-choncholas/mpc3"
  LANGUAGES CXX C)

include(cmake/PreventInSourceBuilds.cmake)
include(ProjectOptions.cmake)


set(TRANSPROJ_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/transputation/")
execute_process(COMMAND make
    WORKING_DIRECTORY "${TRANSPROJ_DIR}")
find_path(TRANSPUTATION_INCLUDE_DIR
  NAMES transputation.h PATHS "${TRANSPROJ_DIR}/transputation"
  REQUIRED NO_CACHE)
find_path(TRANSPUTATION_LIBRARY
  NAMES libtransputation.so PATHS "${TRANSPROJ_DIR}/transputation"
  REQUIRED NO_CACHE)
add_library(transputation SHARED IMPORTED)
set_target_properties(transputation PROPERTIES IMPORTED_LOCATION "${TRANSPUTATION_LIBRARY}/libtransputation.so")
include_directories(SYSTEM "${TRANSPUTATION_INCLUDE_DIR}/..")


find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
foreach(empproj tool)
  set(emp-dir "${CMAKE_CURRENT_SOURCE_DIR}/extern/emp-${empproj}")
  execute_process(COMMAND cmake -DENABLE_FLOAT=On -DTHREADING=On .
    WORKING_DIRECTORY "${emp-dir}")
  execute_process(COMMAND make -j16
    WORKING_DIRECTORY "${emp-dir}")
  execute_process(COMMAND sudo make install
    WORKING_DIRECTORY "${emp-dir}")
  find_package(emp-tool REQUIRED)
endforeach()


mpc3_setup_options()

mpc3_global_options()
include(Dependencies.cmake)
mpc3_setup_dependencies()

mpc3_local_options()

# don't know if this should be set globally from here or not...
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(GIT_SHA
    "Unknown"
    CACHE STRING "SHA this build was generated from")
string(
  SUBSTRING "${GIT_SHA}"
            0
            8
            GIT_SHORT_SHA)

target_compile_features(mpc3_options INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

add_library(mpc3::mpc3_options ALIAS mpc3_options)
add_library(mpc3::mpc3_warnings ALIAS mpc3_warnings)

#add_library(mpc3::mpc3_options INTERFACE IMPORTED)
#add_library(mpc3::mpc3_warnings INTERFACE IMPORTED)

# configure files based on CMake configuration options
add_subdirectory(configured_files)

# Adding the src:
add_subdirectory(src)

# Don't even look at tests if we're not top level
if(NOT PROJECT_IS_TOP_LEVEL)
  return()
endif()

# Adding the tests:
include(CTest)

if(BUILD_TESTING)
  message(AUTHOR_WARNING "Building Tests. Be sure to check out test/constexpr_tests.cpp for constexpr testing")
  add_subdirectory(test)
endif()


if(CMAKE_SKIP_INSTALL_RULES)
  return()
endif()

include(cmake/PackageProject.cmake)

# Add other targets that you want installed here, by default we just package the one executable
# we know we want to ship
mpc3_package_project(
  TARGETS
  net_bench_app
  mpc3_options
  mpc3_warnings
  # FIXME: this does not work! CK
  # PRIVATE_DEPENDENCIES_CONFIGURED project_options project_warnings
)

# Experience shows that explicit package naming can help make it easier to sort
# out potential ABI related issues before they start, while helping you
# track a build to a specific GIT SHA
set(CPACK_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${GIT_SHORT_SHA}-${CMAKE_SYSTEM_NAME}-${CMAKE_BUILD_TYPE}-${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}"
)

include(CPack)
