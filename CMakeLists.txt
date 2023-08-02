cmake_minimum_required( VERSION 3.21.0 )

file( STRINGS VERSION.txt NANODBC_VERSION )
string( REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)"
  NANODBC_VERSION "${NANODBC_VERSION}" )
set( NANODBC_VERSION_MAJOR ${CMAKE_MATCH_1} CACHE STRING "Major Version" )
set( NANODBC_VERSION_MINOR ${CMAKE_MATCH_2} CACHE STRING "Minor Version" )
set( NANODBC_VERSION_PATCH ${CMAKE_MATCH_3} CACHE STRING "Patch Version" )

project( nanodbc
  LANGUAGES CXX
  VERSION ${NANODBC_VERSION}
  DESCRIPTION "nanodbc is a small library that makes ODBC API programming easy and fun"
  HOMEPAGE_URL "https://nanodbc.github.io/nanodbc/"
)

# #######################################
# # nanodbc version
# #######################################

# Default to honoring the visibility settings for static libraries
cmake_policy( SET CMP0063 NEW )

# Default to @rpath on macOS
cmake_policy( SET CMP0042 NEW )

# cmake dependent option supports full syntax
cmake_policy( SET CMP0127 NEW )

# nanodbc specific options
option( NANODBC_DISABLE_ASYNC "Disable async features entirely (default off)" OFF )
option( NANODBC_DISABLE_MSSQL_TVP "Do not use MSSQL Table-valued parameter (default off)" OFF )
option( NANODBC_DISABLE_ASYNC "Disable async features entirely (default off)" OFF )
option( NANODBC_ENABLE_UNICODE "Enable Unicode support (default on)" OFF )
option( NANODBC_ENABLE_WORKAROUND_NODATA "Enable SQL_NO_DATA workaround (see Issue #33) (default off)" OFF )

include( CheckCXXCompilerFlag )
check_cxx_compiler_flag( "-stdlib=libc++" CXX_COMPILER_SUPPORTS_LIBCXX )
check_cxx_compiler_flag( "-Werror" CXX_COMPILER_SUPPORTS_WERROR )
check_cxx_compiler_flag( "--coverage -O0" CXX_COMPILER_SUPPORTS_COVERAGE )

include( CMakeDependentOption )
cmake_dependent_option( NANODBC_FORCE_LIBCXX "Force the use of libc++ (default on if compiler supports it)" ON "CXX_COMPILER_SUPPORTS_LIBCXX" OFF )

cmake_dependent_option( NANODBC_BUILD_EXAMPLES "Build examples (default on)" ON "PROJECT_IS_TOP_LEVEL" OFF)
cmake_dependent_option( NANODBC_GENERATE_INSTALL "Generate install target (default on)" ON "PROJECT_IS_TOP_LEVEL" OFF)
cmake_dependent_option( NANODBC_BUILD_TESTS "Build tests (default on)" ON "PROJECT_IS_TOP_LEVEL" OFF)

cmake_dependent_option( NANODBC_ENABLE_BOOST
  "Use Boost for Unicode string convertions (requires Boost.Locale) (default off if Unicode enabled)" OFF
  "NANODBC_ENABLE_UNICODE" OFF
)

cmake_dependent_option( NANODBC_OVERALLOCATE_CHAR
  "Allocate buffers of sufficient length such that Unicode can be stored in VAR/CHAR columns (default off)" OFF
  "NANODBC_ENABLE_UNICODE" OFF
)

cmake_dependent_option( NANODBC_ENABLE_COVERAGE "Enable code coverage analysis (default off)" OFF
  "CXX_COMPILER_SUPPORTS_COVERAGE AND NANODBC_BUILD_TESTS" OFF
)

cmake_dependent_option( NANODBC_FORCE_WARNINGS_AS_ERROR
  "Treat warnings on nanodbc compile as errors" OFF
  "(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24.0) OR CXX_COMPILER_SUPPORTS_WERROR" OFF
)

message( STATUS "nanodbc version: ${NANODBC_VERSION}" )

########################################
## nanodbc compilation
########################################
# set project-global variables for tests/examples if we're not a subproject
if( PROJECT_IS_TOP_LEVEL )
  set(CMAKE_CXX_STANDARD 14 CACHE STRING "The C++ standard to use for nanodbc")
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()
# ==== CMAKE default vars must be set before target creation! ====
# BUILD_SHARED_LIBS dictates type
add_library( nanodbc nanodbc/nanodbc.cpp )
target_sources(nanodbc
  PUBLIC FILE_SET HEADERS FILES
  nanodbc/nanodbc.h
  $<$<CXX_COMPILER_ID:MSVC>:nanodbc/variant_row_cached_result.h>)
target_sources(nanodbc
  PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:nanodbc/variant_row_cached_result.cpp>)
message( STATUS "nanodbc compile: Global setting: C++${CMAKE_CXX_STANDARD}; nanodbc requires C++14 or greater" )

# nanodbc requires at least C++14, but can use C++17 and beyond
target_compile_features(nanodbc PRIVATE cxx_std_14)

message( STATUS "nanodbc compile: warning-as-error ${NANODBC_FORCE_WARNINGS_AS_ERROR}" )
if( NANODBC_FORCE_WARNINGS_AS_ERROR )
  if (CMAKE_VERSION GREATER_EQUAL 3.24.0)
    set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
  elseif( CXX_COMPILER_SUPPORTS_WERROR )
    target_compile_options( nanodbc PRIVATE -Werror )
  endif()
endif()

message( STATUS "nanodbc build: Force linking libc++ - ${NANODBC_FORCE_LIBCXX}" )
if( NANODBC_FORCE_LIBCXX )
  target_compile_options( nanodbc PUBLIC -stdlib=libc++ )
  target_link_options( nanodbc PUBLIC -stdlib=libc++ -lc++abi )
endif()

set_target_properties( nanodbc PROPERTIES
  VERSION ${NANODBC_VERSION}
  WINDOWS_EXPORT_ALL_SYMBOLS ON
  CXX_EXTENSIONS OFF
)

find_package( ODBC REQUIRED )
target_link_libraries( nanodbc PUBLIC ODBC::ODBC )

if( CMAKE_CXX_COMPILER_ID MATCHES "Intel" )
  target_compile_options( nanodbc PRIVATE
    /QaxCORE-AVX2
    /fp:precise
    "$<$<CONFIG:Debug>:/Od>"
    "$<$<CONFIG:Release>:/O3 /Qipo>"
  )
endif()

# AppleClang complains of unused `-I/path/` arguments.
# These are harmless and can be safely ignored.
target_compile_options( nanodbc PRIVATE
  "$<$<CXX_COMPILER_ID:MSVC,Intel>:/W4>"
  "$<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-unused-command-line-argument>"
)

# #######################################
# # nanodbc features
# #######################################
IF( DEFINED NANODBC_ODBC_VERSION )
  message( STATUS "nanodbc feature: ODBC Version Override - ${NANODBC_ODBC_VERSION}" )
  target_compile_definitions( nanodbc PUBLIC NANODBC_ODBC_VERSION=${NANODBC_ODBC_VERSION} )
else()
  message( STATUS "nanodbc feature: ODBC Version Override - OFF" )
endif()

message( STATUS "nanodbc feature: Enable Unicode - ${NANODBC_ENABLE_UNICODE}" )

if( NANODBC_ENABLE_UNICODE )
  target_compile_definitions( nanodbc PUBLIC
    NANODBC_ENABLE_UNICODE
    "$<$<CXX_COMPILER_ID:MSVC>:UNICODE>"
    "$<$<CXX_COMPILER_ID:MSVC>:_UNICODE>"
  )

  target_compile_definitions( nanodbc PRIVATE
    "$<$<CXX_COMPILER_ID:MSVC>:_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING>"
  )

  message( STATUS "nanodbc feature: Enable Boost - ${NANODBC_ENABLE_BOOST}" )

  if( NANODBC_ENABLE_BOOST )
    find_package( Boost REQUIRED COMPONENTS locale )
    target_link_libraries( nanodbc PUBLIC Boost::locale )
    target_compile_definitions( nanodbc PUBLIC NANODBC_ENABLE_BOOST )
  endif()
endif()

message( STATUS "nanodbc feature: Allocate buffers of sufficient length such that Unicode can be stored in VAR/CHAR columns - ${NANODBC_OVERALLOCATE_CHAR}" )
message( STATUS "nanodbc feature: Enable SQL_NO_DATA bug workaround - ${NANODBC_ENABLE_WORKAROUND_NODATA}" )
message( STATUS "nanodbc feature: Disable async features - ${NANODBC_DISABLE_ASYNC}" )
message( STATUS "nanodbc feature: Disable MSSQL Table-valued parameter - ${NANODBC_DISABLE_MSSQL_TVP}" )

target_compile_definitions( nanodbc PUBLIC
  $<$<BOOL:${NANODBC_DISABLE_MSSQL_TVP}>:NANODBC_DISABLE_MSSQL_TVP>
  $<$<BOOL:${NANODBC_OVERALLOCATE_CHAR}>:NANODBC_OVERALLOCATE_CHAR>
  $<$<BOOL:${NANODBC_ENABLE_WORKAROUND_NODATA}>:NANODBC_ENABLE_WORKAROUND_NODATA>
  $<$<BOOL:${NANODBC_DISABLE_ASYNC}>:NANODBC_DISABLE_ASYNC>
)

# #######################################
# # install targets
# #######################################
message( STATUS "nanodbc build: Generate install target - ${NANODBC_GENERATE_INSTALL}" )

if( NANODBC_GENERATE_INSTALL )
  include( GNUInstallDirs )

  # 'make install' to the correct location
  install( TARGETS nanodbc
      EXPORT nanodbc-config
      ARCHIVE
      LIBRARY
      RUNTIME
      FILE_SET HEADERS
  )

  # Make project importable from the install directory
  # Generate and install *-config.cmake exporting targets from install tree.
  install( EXPORT nanodbc-config
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/nanodbc"
  )
  export( TARGETS nanodbc NAMESPACE nanodbc:: FILE nanodbc-config.cmake )
endif()

# #######################################
# # tests targets
# #######################################
message( STATUS "nanodbc build: Build tests - ${NANODBC_BUILD_TESTS}" )

if( NANODBC_BUILD_TESTS )
  include (CTest)
  list(APPEND CMAKE_CTEST_ARGUMENTS --force-new-ctest-process --output-on-failure )
  add_subdirectory( test )
endif()

message( STATUS "nanodbc build: Enable test coverage - ${NANODBC_ENABLE_COVERAGE}" )

if( NANODBC_ENABLE_COVERAGE )
  target_compile_options( nanodbc PUBLIC --coverage -O0 )
  find_library( gcov gcov )
  target_link_libraries( nanodbc PUBLIC gcov )
endif()

# #######################################
# # examples targets
# #######################################
message( STATUS "nanodbc build: Build examples - ${NANODBC_BUILD_EXAMPLES}" )

if( NANODBC_BUILD_EXAMPLES )
  add_subdirectory( example )
endif()
