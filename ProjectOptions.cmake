include(cmake/SystemLink.cmake)
include(cmake/LibFuzzer.cmake)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)


macro(mpc3_supports_sanitizers)
  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND NOT WIN32)
    set(SUPPORTS_UBSAN ON)
  else()
    set(SUPPORTS_UBSAN OFF)
  endif()

  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND WIN32)
    set(SUPPORTS_ASAN OFF)
  else()
    set(SUPPORTS_ASAN ON)
  endif()
endmacro()

macro(mpc3_setup_options)
  option(mpc3_ENABLE_HARDENING "Enable hardening" ON)
  option(mpc3_ENABLE_COVERAGE "Enable coverage reporting" OFF)
  cmake_dependent_option(
    mpc3_ENABLE_GLOBAL_HARDENING
    "Attempt to push hardening options to built dependencies"
    ON
    mpc3_ENABLE_HARDENING
    OFF)

  mpc3_supports_sanitizers()

  if(NOT PROJECT_IS_TOP_LEVEL OR mpc3_PACKAGING_MAINTAINER_MODE)
    option(mpc3_ENABLE_IPO "Enable IPO/LTO" OFF)
    option(mpc3_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
    option(mpc3_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(mpc3_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    option(mpc3_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(mpc3_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
    option(mpc3_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(mpc3_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(mpc3_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(mpc3_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
    option(mpc3_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(mpc3_ENABLE_PCH "Enable precompiled headers" OFF)
    option(mpc3_ENABLE_CACHE "Enable ccache" OFF)
  else()
    option(mpc3_ENABLE_IPO "Enable IPO/LTO" OFF)
    option(mpc3_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
    option(mpc3_ENABLE_USER_LINKER "Enable user-selected linker" OFF)
    option(mpc3_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ${SUPPORTS_ASAN})
    option(mpc3_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(mpc3_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" ${SUPPORTS_UBSAN})
    option(mpc3_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(mpc3_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(mpc3_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(mpc3_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
    option(mpc3_ENABLE_CPPCHECK "Enable cpp-check analysis" ON)
    option(mpc3_ENABLE_PCH "Enable precompiled headers" OFF)
    option(mpc3_ENABLE_CACHE "Enable ccache" ON)
  endif()

  if(NOT PROJECT_IS_TOP_LEVEL)
    mark_as_advanced(
      mpc3_ENABLE_IPO
      mpc3_WARNINGS_AS_ERRORS
      mpc3_ENABLE_USER_LINKER
      mpc3_ENABLE_SANITIZER_ADDRESS
      mpc3_ENABLE_SANITIZER_LEAK
      mpc3_ENABLE_SANITIZER_UNDEFINED
      mpc3_ENABLE_SANITIZER_THREAD
      mpc3_ENABLE_SANITIZER_MEMORY
      mpc3_ENABLE_UNITY_BUILD
      mpc3_ENABLE_CLANG_TIDY
      mpc3_ENABLE_CPPCHECK
      mpc3_ENABLE_COVERAGE
      mpc3_ENABLE_PCH
      mpc3_ENABLE_CACHE)
  endif()

  mpc3_check_libfuzzer_support(LIBFUZZER_SUPPORTED)
  if(LIBFUZZER_SUPPORTED AND (mpc3_ENABLE_SANITIZER_ADDRESS OR mpc3_ENABLE_SANITIZER_THREAD OR mpc3_ENABLE_SANITIZER_UNDEFINED))
    set(DEFAULT_FUZZER ON)
  else()
    set(DEFAULT_FUZZER OFF)
  endif()

  option(mpc3_BUILD_FUZZ_TESTS "Enable fuzz testing executable" ${DEFAULT_FUZZER})

endmacro()

macro(mpc3_global_options)
  if(mpc3_ENABLE_IPO)
    include(cmake/InterproceduralOptimization.cmake)
    mpc3_enable_ipo()
  endif()

  mpc3_supports_sanitizers()

  if(mpc3_ENABLE_HARDENING AND mpc3_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR mpc3_ENABLE_SANITIZER_UNDEFINED
       OR mpc3_ENABLE_SANITIZER_ADDRESS
       OR mpc3_ENABLE_SANITIZER_THREAD
       OR mpc3_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    message("${mpc3_ENABLE_HARDENING} ${ENABLE_UBSAN_MINIMAL_RUNTIME} ${mpc3_ENABLE_SANITIZER_UNDEFINED}")
    mpc3_enable_hardening(mpc3_options ON ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()
endmacro()

macro(mpc3_local_options)
  if(PROJECT_IS_TOP_LEVEL)
    include(cmake/StandardProjectSettings.cmake)
  endif()

  add_library(mpc3_warnings INTERFACE)
  add_library(mpc3_options INTERFACE)

  include(cmake/CompilerWarnings.cmake)
  mpc3_set_project_warnings(
    mpc3_warnings
    ${mpc3_WARNINGS_AS_ERRORS}
    ""
    ""
    ""
    "")

  if(mpc3_ENABLE_USER_LINKER)
    include(cmake/Linker.cmake)
    configure_linker(mpc3_options)
  endif()

  include(cmake/Sanitizers.cmake)
  mpc3_enable_sanitizers(
    mpc3_options
    ${mpc3_ENABLE_SANITIZER_ADDRESS}
    ${mpc3_ENABLE_SANITIZER_LEAK}
    ${mpc3_ENABLE_SANITIZER_UNDEFINED}
    ${mpc3_ENABLE_SANITIZER_THREAD}
    ${mpc3_ENABLE_SANITIZER_MEMORY})

  set_target_properties(mpc3_options PROPERTIES UNITY_BUILD ${mpc3_ENABLE_UNITY_BUILD})

  if(mpc3_ENABLE_PCH)
    target_precompile_headers(
      mpc3_options
      INTERFACE
      <vector>
      <string>
      <utility>)
  endif()

  if(mpc3_ENABLE_CACHE)
    include(cmake/Cache.cmake)
    mpc3_enable_cache()
  endif()

  include(cmake/StaticAnalyzers.cmake)
  if(mpc3_ENABLE_CLANG_TIDY)
    mpc3_enable_clang_tidy(mpc3_options ${mpc3_WARNINGS_AS_ERRORS})
  endif()

  if(mpc3_ENABLE_CPPCHECK)
    mpc3_enable_cppcheck(${mpc3_WARNINGS_AS_ERRORS} "" # override cppcheck options
    )
  endif()

  if(mpc3_ENABLE_COVERAGE)
    include(cmake/Tests.cmake)
    mpc3_enable_coverage(mpc3_options)
  endif()

  if(mpc3_WARNINGS_AS_ERRORS)
    check_cxx_compiler_flag("-Wl,--fatal-warnings" LINKER_FATAL_WARNINGS)
    if(LINKER_FATAL_WARNINGS)
      # This is not working consistently, so disabling for now
      # target_link_options(mpc3_options INTERFACE -Wl,--fatal-warnings)
    endif()
  endif()

  if(mpc3_ENABLE_HARDENING AND NOT mpc3_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR mpc3_ENABLE_SANITIZER_UNDEFINED
       OR mpc3_ENABLE_SANITIZER_ADDRESS
       OR mpc3_ENABLE_SANITIZER_THREAD
       OR mpc3_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    mpc3_enable_hardening(mpc3_options OFF ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()

endmacro()
