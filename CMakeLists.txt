cmake_minimum_required(VERSION 2.6)

project( GLideN64 )

set(GLideN64_SOURCES
  3DMath.cpp
  Combiner.cpp
  CommonPluginAPI.cpp
  CRC.cpp
  DepthBuffer.cpp
  F3D.cpp
  F3DDKR.cpp
  F3DEX.cpp
  F3DPD.cpp
  F3DWRUS.cpp
  F3DSWSE.cpp
  F3DEX2.cpp
  F3DEX2CBFD.cpp
  FrameBuffer.cpp
  GBI.cpp
  gDP.cpp
  GLideN64.cpp
  GLSLCombiner.cpp
  gSP.cpp
  L3D.cpp
  L3DEX2.cpp
  L3DEX.cpp
  N64.cpp
  OpenGL.cpp
  RDP.cpp
  RSP.cpp
  S2DEX2.cpp
  S2DEX.cpp
  Turbo3D.cpp
  ZSort.cpp
  Textures.cpp
  VI.cpp
  common/CommonAPIImpl_common.cpp
)

include_directories( GLideNHQ )
LINK_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/GLideNHQ/build )

if(MUPENPLUSAPI)
  add_definitions(
	-DMUPENPLUSAPI
  )
  include_directories( inc )
  set(GLideN64_SOURCES_UNIX
	MupenPlusPluginAPI.cpp
	mupenplus/Config_mupenplus.cpp
	mupenplus/CommonAPIImpl_mupenplus.cpp
	mupenplus/MicrocodeDialog_mupenplus.cpp
	mupenplus/MupenPlusAPIImpl.cpp
	mupenplus/OpenGL_mupenplus.cpp
  )
  set(GLideN64_SOURCES_WIN ${GLideN64_SOURCES_UNIX}
	windows/GLFunctions.cpp
  )
else(MUPENPLUSAPI)
  set(GLideN64_SOURCES_WIN
	ZilmarPluginAPI.cpp
	common/ZilmarAPIImpl_common.cpp
	windows/Config_windows.cpp
	windows/Debug.cpp
	windows/CommonAPIImpl_windows.cpp
	windows/GLideN64_windows.cpp
	windows/GLFunctions.cpp
	windows/MicrocodeDialog_windows.cpp
	windows/OpenGL_windows.cpp
	windows/ZilmarAPIImpl_windows.cpp
  )
  set(GLideN64_SOURCES_UNIX
	ZilmarPluginAPI.cpp
	common/ZilmarAPIImpl_common.cpp
	posix/Config_posix.cpp
	posix/Debug.cpp
	posix/CommonAPIImpl_posix.cpp
	posix/MicrocodeDialog_posix.cpp
	posix/OpenGL_posix.cpp
	posix/ZilmarAPIImpl_posix.cpp
  )
endif(MUPENPLUSAPI)

if(UNIX)
  list(APPEND GLideN64_SOURCES ${GLideN64_SOURCES_UNIX})
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	add_definitions(
	  -DOS_MAC_OS_X
	)
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	add_definitions(
	  -DOS_LINUX
	)
  endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  if( NOT MUPENPLUSAPI)
  add_definitions(
	-D_DEBUG
  )
  find_package(PkgConfig)
  pkg_check_modules(GLIB REQUIRED glib-2.0)
  include_directories(${GLIB_INCLUDE_DIRS})
  find_package(GTK2 REQUIRED)
  include_directories(${GTK2_INCLUDE_DIRS})
  endif( NOT MUPENPLUSAPI)
endif(UNIX)

if(WIN32)
  list(APPEND GLideN64_SOURCES ${GLideN64_SOURCES_WIN})
  add_definitions(
	-DOS_WINDOWS
	-D__WIN32__
	-DWIN32
	-D_WIN32_ASM
	-D_CRT_SECURE_NO_WARNINGS
	-D__MSC__
  )
endif(WIN32)

if(SDL)
  include(FindPkgConfig)
  pkg_check_modules(SDL REQUIRED sdl)
  include_directories(${SDL_INCLUDE_DIRS})
  add_definitions(
	-DUSE_SDL
  )
endif(SDL)

if(OPT)
	add_definitions(
	-D__CRC_OPT
	-D__HASHMAP_OPT
	-D__TRIBUFFER_OPT
	-D__VEC4_OPT
	)
endif(OPT)

# Build type

if( NOT CMAKE_BUILD_TYPE)
	set( CMAKE_BUILD_TYPE Release)
endif( NOT CMAKE_BUILD_TYPE)

if( CMAKE_BUILD_TYPE STREQUAL "Debug")
	set( CMAKE_BUILD_TYPE Debug)
	set( DEBUG_BUILD TRUE)
#	add_definitions(
#	-DDEBUG
#	)
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

add_library( GLideN64 SHARED ${GLideN64_SOURCES})

SET_TARGET_PROPERTIES(
  GLideN64
  PROPERTIES
  LINKER_LANGUAGE CXX # Or else we get an error message, because cmake can't figure out from the ".o"-suffix that it is a C-linker we need.
  PREFIX ""
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/plugin
  )

target_link_libraries(GLideN64 ${OPENGL_LIBRARIES} ${SDL_LIBRARIES} GLideNHQ )
