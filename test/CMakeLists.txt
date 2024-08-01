# nanodbc tests build configuration
find_package(ODBC REQUIRED)
if(DEFINED ENV{CI})
  add_compile_definitions(NANODBC_TEST_CI=1)
endif()
add_compile_definitions( "$<$<CXX_COMPILER_ID:MSVC>:_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING>" )

# Prepare "Catch" library for other executables
add_library(Catch INTERFACE)
add_custom_target(tests)

message( STATUS "Building utility tests")
add_executable(utility_tests utility_test.cpp)
target_link_libraries(utility_tests PRIVATE ODBC::ODBC Catch nanodbc)
target_compile_features(utility_tests PRIVATE cxx_std_14)
add_test(NAME utility_tests COMMAND utility_tests)
add_dependencies(tests utility_tests)

# Database-specific tests
set(test_list mssql mysql postgresql sqlite vertica) # odbc_test.cpp is a dummy

foreach(test_item IN LISTS test_list)
  # AppVeyor set common environment variable CI
  # and nanodbc-specific DB (in CI build configuration).
  # The CI builds enable DB test for DB specified in build job configuration.
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

  target_compile_features(${test_name} PRIVATE cxx_std_14)

  set_target_properties(${test_name}
    PROPERTIES
    CXX_EXTENSIONS OFF
    VERSION ${NANODBC_VERSION}
  )

  add_test(NAME ${test_name} COMMAND ${test_name})

  add_dependencies(tests ${test_name})
endforeach()
