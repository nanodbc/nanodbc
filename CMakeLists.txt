cmake_minimum_required(VERSION 3.0.0)
project(nanodbc CXX)

option(BUILD_SHARED_LIBS "Build shared library" OFF)
option(NANODBC_USE_UNICODE "build with unicode support turned on" OFF)
option(NANODBC_USE_BOOST_CONVERT "build using Boost.Locale for string convert" OFF)
option(NANODBC_HANDLE_NODATA_BUG "enable special handling for SQL_NO_DATA (required for vertica)" OFF)
option(NANODBC_DISABLE_ASYNC "disable async features entirely (may help resolve issues in VS2015 builds)" OFF)
option(NANODBC_EXAMPLES "build examples" ON)
option(NANODBC_TEST "build tests" ON)
option(NANODBC_INSTALL "generate install target" ON)
option(NANODBC_ENABLE_LIBCXX "Use libc++ if available." ON)

########################################
## nanodbc version
########################################
file(STRINGS VERSION NANODBC_VERSION REGEX "[0-9]+\\.[0-9]+\\.[0-9]+")
string(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" NANODBC_VERSION_MAJOR "${NANODBC_VERSION}")
string(REGEX REPLACE "^[0-9]+\\.([0-9])+\\.[0-9]+" "\\1" NANODBC_VERSION_MINOR "${NANODBC_VERSION}")
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" NANODBC_VERSION_PATCH "${NANODBC_VERSION}")
message(STATUS "nanodbc version: ${NANODBC_VERSION}")

########################################
## require and enable C++0x/11/14
########################################
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
  include(CheckCXXCompilerFlag)
  if(NANODBC_ENABLE_LIBCXX)
    check_cxx_compiler_flag("-stdlib=libc++" CXX_SUPPORTS_STDLIB)
    if(CXX_SUPPORTS_STDLIB)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
      set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -stdlib=libc++")
    endif()
    if(CXX_SUPPORTS_STDLIB)
        message(STATUS "nanodbc build: Using libc++ - Yes")
    else()
        message(STATUS "nanodbc build: Using libc++ - No")
    endif()
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    string(REGEX REPLACE "[/-]W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    if (NOT (CMAKE_VERSION VERSION_LESS 3.6.0)) # Compiler features for Intel in CMake 3.6+
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Qstd=c++17")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /QaxCORE-AVX2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:precise")
    set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O3")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Qipo")
elseif(MSVC)
  string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  if(MSVC_VERSION LESS 1700)
    message(FATAL_ERROR, "nanodbc requires C++11-compliant compiler")
  endif()
endif()

########################################
## nanodbc features
########################################
IF(NOT DEFINED NANODBC_ODBC_VERSION)
  message(STATUS "nanodbc feature: ODBC Version Override - OFF")
else()
  message(STATUS "nanodbc feature: ODBC Version Override - ${NANODBC_ODBC_VERSION}")
  add_definitions(-DNANODBC_ODBC_VERSION=${NANODBC_ODBC_VERSION})
endif()

if(NANODBC_USE_UNICODE)
  add_definitions(-DNANODBC_USE_UNICODE)
  if(MSVC)
    # Sets "Use Unicode Character Set" property in Visual Studio projects
    add_definitions(-DUNICODE -D_UNICODE)
  endif()
endif()
message(STATUS "nanodbc feature: Unicode support - ${NANODBC_USE_UNICODE}")

if(NANODBC_USE_BOOST_CONVERT)
  add_definitions(-DNANODBC_USE_BOOST_CONVERT)
endif()
message(STATUS "nanodbc feature: Boost string convert - ${NANODBC_USE_BOOST_CONVERT}")

if(NANODBC_HANDLE_NODATA_BUG)
  add_definitions(-DNANODBC_HANDLE_NODATA_BUG)
endif()
message(STATUS "nanodbc feature: Handle SQL_NO_DATA bug - ${NANODBC_HANDLE_NODATA_BUG}")

if(NANODBC_DISABLE_ASYNC)
  message(STATUS "Disable async features: Yes")
  add_definitions(-DNANODBC_DISABLE_ASYNC)
endif()
message(STATUS "nanodbc feature: Disable async features - ${NANODBC_DISABLE_ASYNC}")

########################################
## find unixODBC or iODBC config binary
########################################
if(UNIX)
  # Try to find unixODBC first via odbc_config program.
  find_program(ODBC_CONFIG odbc_config
    $ENV{ODBC_PATH}/bin /usr/bin /usr/local/bin PATHS)
  if(ODBC_CONFIG)
    message(STATUS "nanodbc build: ODBC on Unix - unixODBC")
    set(ODBCLIB odbc)
    execute_process(COMMAND ${ODBC_CONFIG} --include-prefix
      OUTPUT_VARIABLE ODBC_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(ODBC_CFLAGS "-I${ODBC_INCLUDE_DIR}")
    set(CMAKE_FLAGS "${CMAKE_FLAGS} ${ODBC_CFLAGS}")
    execute_process(COMMAND ${ODBC_CONFIG} --libs
      OUTPUT_VARIABLE ODBC_LINK_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()

  # Fallback to finding unixODBC via install paths
  if(NOT ODBC_CONFIG)
    find_path(UnixODBC_INCLUDE_DIR uodbc_stats.h
      /usr/include
      /usr/local/include
      /usr/include/odbc
      /usr/local/include/odbc
      /usr/include/libodbc
      /usr/local/include/libodbc)
    if(UnixODBC_INCLUDE_DIR)
      set(ODBC_CONFIG 1)
      message(STATUS "nanodbc build: ODBC on Unix - unixODBC")
      set(ODBCLIB odbc)
      set(ODBC_CFLAGS "-I${UnixODBC_INCLUDE_DIR} -DHAVE_UNISTD_H -DHAVE_PWD_H -DHAVE_SYS_TYPES_H -DHAVE_LONG_LONG -DSIZEOF_LONG_INT=8")
    endif()
  endif()

  # Fallback to using iODBC
  if(NOT ODBC_CONFIG)
    find_program(ODBC_CONFIG iodbc-config
      $ENV{ODBC_PATH}/bin /usr/bin /usr/local/bin PATHS)
    if(ODBC_CONFIG)
      message(STATUS "nanodbc build: ODBC on Unix - iODBC")
      set(ODBCLIB iodbc)
      execute_process(COMMAND ${ODBC_CONFIG} --cflags
        OUTPUT_VARIABLE ODBC_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
      set(CMAKE_FLAGS "${CMAKE_FLAGS} ${ODBC_CFLAGS}")
      execute_process(COMMAND ${ODBC_CONFIG} --libs
        OUTPUT_VARIABLE ODBC_LINK_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
      if(NANODBC_USE_UNICODE)
        add_definitions(-DNANODBC_USE_IODBC_WIDE_STRINGS)
      endif()
    endif()
  endif()

  if(NOT ODBC_CONFIG)
    message(FATAL_ERROR "can not find a suitable odbc driver manager")
  endif()

  message(STATUS "ODBC compile flags: ${ODBC_CFLAGS}")
  message(STATUS "ODBC link flags: ${ODBC_LINK_FLAGS}")
endif()

########################################
## find ODBC libraries to link
########################################
if(UNIX)
  set(ODBC_LIBRARIES ${ODBCLIB})
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${ODBC_LINK_FLAGS}")
elseif(MSVC OR CMAKE_CXX_COMPILER_ID MATCHES "Intel")
  set(ODBC_LIBRARIES odbc32.lib odbccp32.lib Ws2_32.lib)
elseif(MINGW)
  set(ODBC_LIBRARIES odbc32 odbccp32)
endif()

########################################
## find Boost if necessary
########################################
if(NANODBC_USE_BOOST_CONVERT)
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_MULTITHREADED ON)
  find_package(Boost COMPONENTS locale REQUIRED)
  if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${CMAKE_BINARY_DIR}/lib ${Boost_LIBRARY_DIRS})
  else()
    message(FATAL_ERROR "can not find boost")
  endif()
endif()

########################################
## Mac OS X specifics for targets
########################################
if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
  message(STATUS "Use rpaths on Mac OS X - ${CMAKE_MACOSX_RPATH}")

  # AppleClang complains of unused `-I/path/` arguments.
  # These are harmless and can be safely ignored.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument")
endif()

########################################
## library target
########################################
if(BUILD_SHARED_LIBS)
  message(STATUS "nanodbc build: Enable nanodbc target - SHARED")
else()
  message(STATUS "nanodbc build: Enable nanodbc target - STATIC")
endif()

if(WIN32 AND NOT MINGW AND BUILD_SHARED_LIBS)
  message(FATAL_ERROR "Building shared libraries on Windows needs MinGW")
endif()

add_library(nanodbc nanodbc/nanodbc.cpp nanodbc/nanodbc.h)

target_link_libraries(nanodbc ${Boost_LIBRARIES} ${ODBC_LIBRARIES})

target_include_directories(nanodbc PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/nanodbc>) # <prefix>/include/nanodbc

if(UNIX)
  set_target_properties(nanodbc PROPERTIES
    COMPILE_FLAGS "${ODBC_CFLAGS}"
    LIBRARY_OUTPUT_DIRECTORY "lib")
endif()

if(BUILD_SHARED_LIBS)
  set_target_properties(nanodbc PROPERTIES
    VERSION ${NANODBC_VERSION})
endif()

########################################
## install targets
########################################
if(NANODBC_INSTALL)
  set(NANODBC_CONFIG nanodbc-config)
  # 'make install' to the correct location
  if(BUILD_SHARED_LIBS)
    install(TARGETS nanodbc
      EXPORT ${NANODBC_CONFIG} # associate installed target files with export
      INCLUDES DESTINATION include
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
      RUNTIME DESTINATION bin) # for Windows
  else()
    install(TARGETS nanodbc
      EXPORT ${NANODBC_CONFIG} # associate installed target files with export
      INCLUDES DESTINATION include
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib)
  endif()
  # Install public include headers
  install(FILES nanodbc/nanodbc.h DESTINATION include/nanodbc)
  # Make project importable from the install directory
  ## Generate and install *-config.cmake exporting targets from install tree.
  install(EXPORT ${NANODBC_CONFIG} DESTINATION cmake)
  # Make project importable from the build directory
  ## Generate file *-config.cmake exporting targets from build tree.
  export(TARGETS nanodbc FILE ${NANODBC_CONFIG}.cmake)
endif()
message(STATUS "nanodbc build: Enable install target - ${NANODBC_INSTALL}")

########################################
## tests targets
########################################
if(NANODBC_TEST)
  enable_testing()
  add_subdirectory(test)
  add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process --output-on-failure
    DEPENDS tests)
endif()
message(STATUS "nanodbc build: Enable tests target - ${NANODBC_TEST}")

########################################
## examples targets
########################################
if(NANODBC_EXAMPLES)
  add_subdirectory(example)
endif()
message(STATUS "nanodbc build: Enable examples target - ${NANODBC_EXAMPLES}")
