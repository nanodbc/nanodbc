# # nanodbc examples build configuration
find_package(ODBC REQUIRED)
add_compile_definitions( "$<$<CXX_COMPILER_ID:MSVC>:_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING>" )

add_custom_target( examples )

set( example_list empty northwind usage rowset_iteration table_schema table_valued_parameter )

# Generate an empty.cpp from the template, for experimentation
# empty.cpp is ignored by git
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/empty.cpp) # Behavior is well-defined only for full paths.
  configure_file(empty.cpp.in ${CMAKE_CURRENT_SOURCE_DIR}/empty.cpp COPYONLY)
endif()

foreach( example IN LISTS example_list )
    add_executable( example_${example} ${example}.cpp example_unicode_utils.h )
    target_link_libraries( example_${example} PRIVATE nanodbc ODBC::ODBC )
    target_compile_features(example_${example} PRIVATE cxx_std_14)
    set_target_properties(example_${example}
      PROPERTIES
      CXX_EXTENSIONS OFF
      VERSION ${NANODBC_VERSION}
    )
    add_dependencies( examples example_${example})
endforeach()
