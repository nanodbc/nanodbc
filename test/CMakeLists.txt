# nanodbc tests build configuration
find_package(ODBC REQUIRED)
if(DEFINED ENV{CI})
  add_compile_definitions(NANODBC_TEST_CI=1)
endif()
add_compile_definitions( "$<$<CXX_COMPILER_ID:MSVC>:_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING>" )

add_library(Catch STATIC catch/catch_amalgamated.cpp)
target_compile_definitions(Catch PRIVATE CATCH_AMALGAMATED_CUSTOM_MAIN)
target_include_directories(Catch PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(Catch PUBLIC cxx_std_17)

if(NANODBC_FORCE_LIBCXX)
  target_compile_options(Catch PUBLIC -stdlib=libc++)
  target_link_options(Catch PUBLIC -stdlib=libc++ -lc++abi)
endif()

add_custom_target(tests)

message( STATUS "Building utility tests")
add_executable(utility_tests utility_test.cpp)
target_link_libraries(utility_tests PRIVATE ODBC::ODBC Catch nanodbc)
target_compile_features(utility_tests PRIVATE cxx_std_17)
add_test(NAME utility_tests COMMAND utility_tests)
add_dependencies(tests utility_tests)

set(test_list clickhouse firebird mssql mysql oracle postgresql sqlite vertica)

foreach(test_item IN LISTS test_list)
  # The Firebird driver's Unicode entry points cannot open a connection, so that suite is
  # built for a narrow configuration only.
  if(test_item STREQUAL "firebird" AND NANODBC_ENABLE_UNICODE)
    message(STATUS "Skipping firebird tests in a Unicode build")
    continue()
  endif()
  if (DEFINED ENV{CI} AND DEFINED ENV{DB})
    string(TOLOWER $ENV{DB} test_db)
    string(FIND ${test_db} ${test_item} test_found)
    if (test_found LESS 0)
      message(TRACE "Skipping ${test_item} tests")
      continue()
    endif()
  endif()
  message( STATUS "Building ${test_item} tests")
  set(test_name ${test_item}_tests)
  add_executable(${test_name}
      main.cpp
      ${test_item}_test.cpp
      base_test_fixture.h
      test_case_fixture.h
      )
  target_link_libraries(${test_name} PRIVATE Catch ODBC::ODBC nanodbc)

  target_compile_definitions(${test_name}
    PRIVATE
      NANODBC_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/data"
      $<$<CXX_COMPILER_ID:MSVC>:_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING>
  )

  target_compile_options(${test_name}
    PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wno-unknown-pragmas>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-error=parentheses>
  )

  target_compile_features(${test_name} PRIVATE cxx_std_17)

  set_target_properties(${test_name}
    PROPERTIES
    CXX_EXTENSIONS OFF
    VERSION ${NANODBC_VERSION}
  )

  add_test(NAME ${test_name} COMMAND ${test_name})

  add_dependencies(tests ${test_name})
endforeach()
