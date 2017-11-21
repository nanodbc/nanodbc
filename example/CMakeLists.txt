# nanodbc examples build configuration

add_custom_target(examples DEPENDS example)
set(examples northwind usage rowset_iteration table_schema)

include_directories(${CMAKE_SOURCE_DIR} ${ODBC_INCLUDE_DIR})
link_directories(${CMAKE_BINARY_DIR}/lib)

foreach(example ${examples})
  add_executable(example_${example} ${example}.cpp example_unicode_utils.h)
  if (BUILD_SHARED_LIBS)
    target_link_libraries(example_${example} nanodbc "${ODBC_LINK_FLAGS}")
  else()
    target_link_libraries(example_${example} nanodbc ${ODBC_LIBRARIES})
  endif()
  add_dependencies(examples example_${example})

  set_target_properties(example_${example}
    PROPERTIES
    VERSION ${NANODBC_VERSION})
endforeach()

set(example_empty ${CMAKE_CURRENT_SOURCE_DIR}/empty.cpp)
if(NOT EXISTS ${example_empty}) # Behavior is well-defined only for full paths.
  configure_file(${example_empty}.in ${example_empty} COPYONLY)
endif()
add_executable(example_empty ${example_empty} example_unicode_utils.h)
if (BUILD_SHARED_LIBS)
  target_link_libraries(example_empty nanodbc "${ODBC_LINK_FLAGS}")
else()
  target_link_libraries(example_empty nanodbc ${ODBC_LIBRARIES})
endif()
add_dependencies(examples example_empty)

set_target_properties(example_empty
  PROPERTIES
  VERSION ${NANODBC_VERSION})