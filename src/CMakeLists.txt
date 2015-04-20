cmake_minimum_required(VERSION 2.6)

project( GLideN64 )

set(GLideN64_SOURCES
  3DMath.cpp
  Combiner.cpp
  CommonPluginAPI.cpp
  Config.cpp
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
  glState.cpp
  gSP.cpp
  Keys.cpp
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
  TextDrawer.cpp
  PostProcessor.cpp
  VI.cpp
  common/CommonAPIImpl_common.cpp
)

if(MUPENPLUSAPI)
  add_definitions(
	-DMUPENPLUSAPI
		-DTXFILTER_LIB
  )
  include_directories( inc )
  set(GLideN64_SOURCES_UNIX
	MupenPlusPluginAPI.cpp
	mupenplus/Config_mupenplus.cpp
	mupenplus/CommonAPIImpl_mupenplus.cpp
	mupenplus/MupenPlusAPIImpl.cpp
	mupenplus/OpenGL_mupenplus.cpp
  )
  set(GLideN64_SOURCES_WIN ${GLideN64_SOURCES_UNIX}
	windows/GLFunctions_windows.cpp
  )
  set(GLideN64_DLL_NAME mupen64plus-video-GLideN64)
else(MUPENPLUSAPI)
  if(NOT UNIX)
	message(ERROR "UNIX build requires MUPENPLUSAPI!")
  endif(NOT UNIX)
  set(GLideN64_SOURCES_WIN
	ZilmarPluginAPI.cpp
	windows/Config_windows.cpp
	windows/CommonAPIImpl_windows.cpp
	windows/GLideN64_windows.cpp
	windows/GLFunctions.cpp
	windows/OpenGL_windows.cpp
	windows/ZilmarAPIImpl_windows.cpp
  )
  set(GLideN64_DLL_NAME GLideN64)
endif(MUPENPLUSAPI)

add_subdirectory(GLideNHQ)
include_directories( GLideNHQ )

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

SET( ENV{FREETYPE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../freetype )
FIND_PACKAGE( Freetype REQUIRED )
include_directories( ${FREETYPE_INCLUDE_DIRS} )

if(OPT)
	add_definitions(
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

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	SET(GCC_CPP11_COMPILE_FLAGS "-std=c++0x -static-libgcc -static-libstdc++")
	SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_CPP11_COMPILE_FLAGS}" )
	SET(GCC_STATIC_LINK_FLAGS "-static-libgcc -static-libstdc++")
	SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_STATIC_LINK_FLAGS}" )
endif()

add_library( ${GLideN64_DLL_NAME} SHARED ${GLideN64_SOURCES})

if( CMAKE_BUILD_TYPE STREQUAL "Debug")
  SET_TARGET_PROPERTIES(
	${GLideN64_DLL_NAME}
	PROPERTIES
	LINKER_LANGUAGE CXX # Or else we get an error message, because cmake can't figure out from the ".o"-suffix that it is a C-linker we need.
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/plugin/debug
  )

  if(SDL)
	target_link_libraries(${GLideN64_DLL_NAME} PRIVATE ${OPENGL_LIBRARIES} ${SDL_LIBRARIES} ${FREETYPE_LIBRARIES} GLideNHQd )
  else(SDL)
	target_link_libraries(${GLideN64_DLL_NAME} PRIVATE ${OPENGL_LIBRARIES} ${FREETYPE_LIBRARIES} GLideNHQd )
  endif(SDL)
endif( CMAKE_BUILD_TYPE STREQUAL "Debug")

if( CMAKE_BUILD_TYPE STREQUAL "Release")
  SET_TARGET_PROPERTIES(
	${GLideN64_DLL_NAME}
	PROPERTIES
	LINKER_LANGUAGE CXX # Or else we get an error message, because cmake can't figure out from the ".o"-suffix that it is a C-linker we need.
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/plugin/release
  )

  if(SDL)
	target_link_libraries(${GLideN64_DLL_NAME} ${OPENGL_LIBRARIES} ${SDL_LIBRARIES} ${FREETYPE_LIBRARIES} GLideNHQ )
  else(SDL)
	target_link_libraries(${GLideN64_DLL_NAME} PRIVATE ${OPENGL_LIBRARIES} ${FREETYPE_LIBRARIES} GLideNHQ )
  endif(SDL)
endif( CMAKE_BUILD_TYPE STREQUAL "Release")

