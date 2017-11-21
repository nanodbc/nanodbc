cmake_minimum_required(VERSION 3.0.0)
include(ExternalProject)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_COMPILER_IS_GNUCXX)
  # Workaround for Travis CI + Catch: `error: ignoring #pragma gcc diagnostic`
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # Workaround for Travis CI|AppVeyor + Catch: `suggest parentheses around comparison in operand of '=='`
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=parentheses")
endif()

find_package(Git REQUIRED)
ExternalProject_Add(
  catch
  PREFIX ${CMAKE_BINARY_DIR}/catch
  GIT_REPOSITORY https://github.com/catchorg/Catch.git
  GIT_TAG "Catch1.x"
  TIMEOUT 10
  UPDATE_COMMAND ${GIT_EXECUTABLE} pull
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD ON)
ExternalProject_Get_Property(catch source_dir)
set(CATCH_INCLUDE_DIR ${source_dir}/include CACHE INTERNAL "Path to include folder for Catch")

include_directories(${CMAKE_SOURCE_DIR} ${CATCH_INCLUDE_DIR} ${ODBC_INCLUDE_DIR})
link_directories(${CMAKE_BINARY_DIR}/lib)
file(GLOB headers *.h *.hpp)
add_custom_target(tests DEPENDS tests catch)
set(test_list mssql mysql postgresql sqlite vertica) # odbc_test.cpp is a dummy

# Travis CI and AppVeyor set common environment variable CI
# and nanodbc-specific DB (in CI build configuration).
# The CI builds enable DB test for DB specified in build job configuration.
if (DEFINED ENV{CI} AND DEFINED ENV{DB})
  if ($ENV{CI})
    foreach(test_item ${test_list})
      string(TOLOWER $ENV{DB} test_db)
      string(FIND ${test_db} ${test_item} test_found)
      if (test_found LESS 0)
        list(REMOVE_ITEM test_list ${test_item})
      endif()
    endforeach()
    if (NOT test_list)
      message(FATAL_ERROR "CI build misconfigured: no tests specified")
    endif()
  endif()
endif()

foreach(test_item ${test_list})
  set(test_name ${test_item}_tests)
  add_executable(${test_name} main.cpp ${test_item}_test.cpp ${headers})
  add_dependencies(${test_name} catch)
  if (BUILD_SHARED_LIBS)
    target_link_libraries(${test_name} nanodbc "${ODBC_LINK_FLAGS}")
  else()
    target_link_libraries(${test_name} nanodbc ${ODBC_LIBRARIES})
  endif()
  set_target_properties(${test_name}
    PROPERTIES
    VERSION ${NANODBC_VERSION})
  add_test(NAME ${test_name} COMMAND ${test_name})

  set_tests_properties(${test_name} PROPERTIES
    ENVIRONMENT "TEST_NANODBC_DATADIR=${CMAKE_CURRENT_SOURCE_DIR}/data")

  if(NOT CMAKE_GENERATOR MATCHES "^Visual Studio")
    add_custom_target(${test_item}_test
      COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process --output-on-failure -R ${test_name})
    add_custom_target(${test_item}_check
      COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process --output-on-failure -R ${test_name}
      DEPENDS ${test_name})
  endif()
  add_dependencies(tests ${test_name})
endforeach()
