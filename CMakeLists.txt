cmake_minimum_required( VERSION 3.21.0 )
project( nanodbc CXX )

# Default to honoring the visibility settings for static libraries
cmake_policy( SET CMP0063 NEW )

# Default to @rpath on macOS
cmake_policy( SET CMP0042 NEW )

# cmake dependent option supports full syntax
cmake_policy(SET CMP0127 NEW)

add_library( nanodbc
  STATIC
  nanodbc/nanodbc.cpp
  nanodbc/nanodbc.h )

# nanodbc specific options
option( NANODBC_DISABLE_ASYNC "Disable async features entirely" OFF )
option( NANODBC_ENABLE_UNICODE "Enable Unicode support" ON )
option( NANODBC_ENABLE_WORKAROUND_NODATA "Enable SQL_NO_DATA workaround (see Issue #33)" OFF )

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-stdlib=libc++" CXX_SUPPORTS_STDLIB)
check_cxx_compiler_flag("-Werror" CXX_SUPPORTS_WERROR)

include( CMakeDependentOption )
cmake_dependent_option( NANODBC_FORCE_LIBCXX "Force the use of libc++" ON "CXX_SUPPORTS_STDLIB" OFF)



cmake_dependent_option( NANODBC_ENABLE_BOOST
  "Use Boost for Unicode string convertions (requires Boost.Locale)" ON
  "NANODBC_ENABLE_UNICODE" OFF
)
cmake_dependent_option( NANODBC_ENABLE_COVERAGE "Enable code coverage analysis" OFF
"CMAKE_CXX_COMPILER_ID MATCHES Clang OR CMAKE_COMPILER_IS_GNUCXX" OFF
)

cmake_dependent_option( NANODBC_WARNINGS_AS_ERROR
  "Treat warnings on nanodbc compile as errors" ON
  "(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24.0) AND CXX_SUPPORTS_WERROR" OFF
)


if( NANODBC_FORCE_LIBCXX )
  target_compile_options(nanodbc PRIVATE -stdlib=libc++)
  target_link_options(nanodbc PRIVATE -stdlib=libc++)
endif()
message(STATUS "nanodbc build: Force linking libc++ - ${NANODBC_FORCE_LIBCXX}")


if( NANODBC_WARNINGS_AS_ERROR )
  target_compile_options(nanodbc PRIVATE -Werror)
  message( STATUS "nanodbc compile: warning-as-error ${NANODBC_WARNINGS_AS_ERROR}" )
else()
  message( STATUS "nanodbc compile: warning-as-error toolset defaults" )
endif()

# #######################################
# # nanodbc version
# #######################################
file( STRINGS VERSION.txt NANODBC_VERSION REGEX "[0-9]+\\.[0-9]+\\.[0-9]+" )
string( REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" NANODBC_VERSION_MAJOR "${NANODBC_VERSION}" )
string( REGEX REPLACE "^[0-9]+\\.([0-9])+\\.[0-9]+" "\\1" NANODBC_VERSION_MINOR "${NANODBC_VERSION}" )
string( REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" NANODBC_VERSION_PATCH "${NANODBC_VERSION}" )
message( STATUS "nanodbc version: ${NANODBC_VERSION}" )

# #######################################
# # require and enable C++20
# #######################################
message( STATUS "nanodbc compile: C++17" )
target_compile_features( nanodbc PUBLIC cxx_std_17 )
set_target_properties( nanodbc PROPERTIES CXX_EXTENSIONS OFF )


if( NANODBC_ENABLE_COVERAGE )
  target_compile_options( nanodbc PRIVATE --coverage -O0 )
  target_link_libraries( nanodbc PRIVATE gcov )
  message( STATUS "nanodbc build: Enable test coverage - Yes" )
endif()


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
  target_compile_definitions( nanodbc PRIVATE NANODBC_ODBC_VERSION=${NANODBC_ODBC_VERSION} )
else()
  message( STATUS "nanodbc feature: ODBC Version Override - OFF" )
endif()

message( STATUS "nanodbc feature: Enable Unicode - ${NANODBC_ENABLE_UNICODE}" )

if( NANODBC_ENABLE_UNICODE )
  target_compile_definitions( nanodbc PRIVATE NANODBC_ENABLE_UNICODE )

  if( MSVC )
    # Sets "Use Unicode Character Set" property in Visual Studio projects
    target_compile_definitions( nanodbc PRIVATE UNICODE _UNICODE )
  endif()

  message( STATUS "nanodbc feature: Enable Boost - ${NANODBC_ENABLE_BOOST}" )

  if( NANODBC_ENABLE_BOOST )
    find_package( Boost REQUIRED COMPONENTS locale )
    target_link_libraries( nanodbc PUBLIC Boost::locale )
    target_compile_definitions( nanodbc PRIVATE NANODBC_ENABLE_BOOST )
  endif()
endif()

target_compile_definitions( nanodbc PRIVATE
  $<$<BOOL:${NANODBC_ENABLE_WORKAROUND_NODATA}>:NANODBC_ENABLE_WORKAROUND_NODATA>
  $<$<BOOL:${NANODBC_DISABLE_ASYNC}>:NANODBC_DISABLE_ASYNC>
)

message( STATUS "nanodbc feature: Disable async features - ${NANODBC_DISABLE_ASYNC}" )
message( STATUS "nanodbc feature: Enable SQL_NO_DATA bug workaround - ${NANODBC_ENABLE_WORKAROUND_NODATA}" )

find_package( ODBC REQUIRED )
target_link_libraries( nanodbc PUBLIC ODBC::ODBC )

# #######################################
# # library target
# #######################################
target_include_directories( nanodbc PUBLIC SYSTEM
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/nanodbc> ) # <prefix>/include/nanodbc
