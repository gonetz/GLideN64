cmake_minimum_required(VERSION 2.6)

project( GLideNHQ )

set(GLideNHQ_SOURCES
  Ext_TxFilter.cpp
  TextureFilters.cpp
  TextureFilters_2xsai.cpp
  TextureFilters_hq2x.cpp
  TextureFilters_hq4x.cpp
  TxCache.cpp
  TxDbg.cpp
  TxFilter.cpp
  TxFilterExport.cpp
  TxHiResCache.cpp
  TxImage.cpp
  TxQuantize.cpp
  TxReSample.cpp
  TxTexCache.cpp
  TxUtil.cpp
)

include_directories( inc )

set(Boost_USE_STATIC_LIBS   ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost COMPONENTS thread filesystem system REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

if(WIN32)
  LINK_DIRECTORIES( lib )
  add_definitions(
	-DWIN32
	-D_CRT_SECURE_NO_WARNINGS
  )
endif(WIN32)


if (WIN32 AND __COMPILER_GNU)
   # mingw-gcc fails to link boost::thread
   add_definitions(-DBOOST_THREAD_USE_LIB)
endif (WIN32 AND __COMPILER_GNU)

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

find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
link_directories(${OpenGL_LIBRARY_DIRS})
add_definitions(${OpenGL_DEFINITIONS})
if(NOT OPENGL_FOUND)
	message(ERROR " OPENGL not found!")
endif(NOT OPENGL_FOUND)

SET(GCC_CPP11_COMPILE_FLAGS "-std=c++0x -static-libgcc -static-libstdc++")
SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_CPP11_COMPILE_FLAGS}" )
SET(GCC_STATIC_LINK_FLAGS "-static-libgcc -static-libstdc++")
SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_STATIC_LINK_FLAGS}" )

add_library( GLideNHQ STATIC ${GLideNHQ_SOURCES})

SET_TARGET_PROPERTIES(
  GLideNHQ
  PROPERTIES
  LINKER_LANGUAGE CXX # Or else we get an error message, because cmake can't figure out from the ".o"-suffix that it is a C-linker we need.
  PREFIX ""
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/plugin
  )

if( CMAKE_BUILD_TYPE STREQUAL "Debug")
target_link_libraries(GLideNHQ ${OPENGL_LIBRARIES} ${Boost_LIBRARIES} libpng zlib )
endif( CMAKE_BUILD_TYPE STREQUAL "Debug")

if( CMAKE_BUILD_TYPE STREQUAL "Release")
target_link_libraries(GLideNHQ ${OPENGL_LIBRARIES} ${Boost_LIBRARIES} libpngd zlibd )
endif( CMAKE_BUILD_TYPE STREQUAL "Debug")
