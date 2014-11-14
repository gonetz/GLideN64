cmake_minimum_required(VERSION 2.6)

project( dxtn )

set(DXTN_SOURCES
  dxtn.c
  texstore.c
  wrapper.c
)

# Build type

if( NOT CMAKE_BUILD_TYPE)
	set( CMAKE_BUILD_TYPE Release)
endif( NOT CMAKE_BUILD_TYPE)

if( CMAKE_BUILD_TYPE STREQUAL "Debug")
	set( CMAKE_BUILD_TYPE Debug)
	set( DEBUG_BUILD TRUE)
	add_definitions(
		-DDEBUG
	)
endif( CMAKE_BUILD_TYPE STREQUAL "Debug")


add_library( dxtn STATIC ${DXTN_SOURCES})

SET_TARGET_PROPERTIES(
  dxtn
  PROPERTIES
  LINKER_LANGUAGE C # Or else we get an error message, because cmake can't figure out from the ".o"-suffix that it is a C-linker we need.
  PREFIX ""
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib
)

