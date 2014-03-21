cmake_minimum_required(VERSION 2.6)

project( GLideN64 )

set(GLideN64_SOURCES 
  2xSAI.cpp
  3DMath.cpp
  Combiner.cpp
  CRC.cpp
  Debug.cpp
  Debug_linux.cpp
  DepthBuffer.cpp
  F3D.cpp
  F3DDKR.cpp
  F3DEX2.cpp
  F3DEX.cpp
  F3DPD.cpp
  F3DWRUS.cpp
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
  Textures.cpp
  VI.cpp
)

if(MUPENPLUSAPI)
  add_definitions(
    -DMUPENPLUSAPI
  )
  include_directories( inc )
  set(GLideN64_SOURCES_WIN
    Config_mupen.cpp
  )
  set(GLideN64_SOURCES_LINUX
    Config_mupen.cpp
  )
else(MUPENPLUSAPI)
  set(GLideN64_SOURCES_WIN
    Config.cpp
  )
  set(GLideN64_SOURCES_LINUX
    Config_linux.cpp
  )
endif(MUPENPLUSAPI)

if(UNIX)
  list(APPEND GLideN64_SOURCES ${GLideN64_SOURCES_LINUX})
  add_definitions(
	-D__LINUX__
	-D_DEBUG
  )
  find_package(PkgConfig)
  pkg_check_modules(GLIB REQUIRED glib-2.0)
  include_directories(${GLIB_INCLUDE_DIRS})
  find_package(GTK2 REQUIRED)
  include_directories(${GTK2_INCLUDE_DIRS})
endif(UNIX)

if(WIN32)
  list(APPEND GLideN64_SOURCES ${GLideN64_SOURCES_WIN})
  add_definitions( 
    -DWIN32
    -DWINDOWS
    -D__WIN32__
    -D_WIN32
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

find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
link_directories(${OpenGL_LIBRARY_DIRS})
add_definitions(${OpenGL_DEFINITIONS})
if(NOT OPENGL_FOUND)
	message(ERROR " OPENGL not found!")
endif(NOT OPENGL_FOUND)

add_library( GLideN64 SHARED ${GLideN64_SOURCES})

SET_TARGET_PROPERTIES(
  GLideN64
  PROPERTIES
  LINKER_LANGUAGE CXX # Or else we get an error message, because cmake can't figure out from the ".o"-suffix that it is a C-linker we need.
  PREFIX ""
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/plugin
  )

target_link_libraries(GLideN64 ${OPENGL_LIBRARIES} ${SDL_LIBRARIES})
