cmake_minimum_required(VERSION 2.6)

option(GLES2 "Set to ON if targeting a GLES2 device" ${GLES2})
option(PANDORA "Set to ON if targeting an OpenPandora" ${PANDORA})
option(MUPENPLUSAPI "Set to ON for Mupen64Plus plugin" ${MUPENPLUSAPI})

project( GLideN64 )

set(GLideN64_SOURCES
  3DMath.cpp
  Combiner.cpp
  CommonPluginAPI.cpp
  Config.cpp
  convert.cpp
  CRC32.cpp
  DepthBuffer.cpp
  F3D.cpp
  F3DDKR.cpp
  F3DEX.cpp
  F3DPD.cpp
  F3DWRUS.cpp
  F3DSWSE.cpp
  F3DEX2.cpp
  F3DEX2CBFD.cpp
  FBOTextureFormats.cpp
  FrameBuffer.cpp
  FrameBufferInfo.cpp
  GBI.cpp
  gDP.cpp
  GLideN64.cpp
  glState.cpp
  gSP.cpp
  Keys.cpp
  L3D.cpp
  L3DEX2.cpp
  L3DEX.cpp
  N64.cpp
  OpenGL.cpp
  Performance.cpp
  RDP.cpp
  RSP.cpp
  S2DEX2.cpp
  S2DEX.cpp
  Turbo3D.cpp
  ZSort.cpp
  ShaderUtils.cpp
  SoftwareRender.cpp
  Textures.cpp
  TextDrawer.cpp
  TextureFilterHandler.cpp
  PostProcessor.cpp
  VI.cpp
  BufferCopy/ColorBufferToRDRAM.cpp
  BufferCopy/DepthBufferToRDRAM.cpp
  BufferCopy/RDRAMtoColorBuffer.cpp
  DepthBufferRender/ClipPolygon.cpp
  DepthBufferRender/DepthBufferRender.cpp
  common/CommonAPIImpl_common.cpp
)

#check if we're running on Raspberry Pi
MESSAGE("Looking for bcm_host.h")
if(EXISTS "/opt/vc/include/bcm_host.h")
  MESSAGE("bcm_host.h found")
  set(BCMHOST ON)
  set(GLES2 ON)
  add_definitions(
    -DVC
    -DUSE_DEPTH_RENDERBUFFER
  )
  include_directories(
    "/opt/vc/include"
    "/opt/vc/include/interface/vcos"
    "/opt/vc/include/interface/vmcs_host/linux"
    "/opt/vc/include/interface/vcos/pthreads"
  )
  link_directories(
    "/opt/vc/lib"
    "/opt/vc/lib/GL"
  )
else(EXISTS "/opt/vc/include/bcm_host.h")
  MESSAGE("bcm_host.h not found")
endif(EXISTS "/opt/vc/include/bcm_host.h")

if(MUPENPLUSAPI)
  add_definitions(
	-DMUPENPLUSAPI
		-DTXFILTER_LIB
  )
  include_directories( . inc )
  set(GLideN64_SOURCES_UNIX
	MupenPlusPluginAPI.cpp
	mupenplus/Config_mupenplus.cpp
	mupenplus/CommonAPIImpl_mupenplus.cpp
	mupenplus/MupenPlusAPIImpl.cpp
	mupenplus/OpenGL_mupenplus.cpp
  )

  set(GLideN64_SOURCES_WIN ${GLideN64_SOURCES_UNIX}
	common/GLFunctions.cpp
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
	common/GLFunctions.cpp
	windows/OpenGL_windows.cpp
	windows/ZilmarAPIImpl_windows.cpp
  )
  set(GLideN64_DLL_NAME GLideN64)
endif(MUPENPLUSAPI)

add_subdirectory( osal )
include_directories( osal )

if (NOHQ)
  list(APPEND GLideN64_SOURCES
    TxFilterStub.cpp
  )
else (NOHQ)
  add_subdirectory( GLideNHQ )
  include_directories( GLideNHQ )
endif(NOHQ)

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

if(PANDORA)
  #Pandora as a SGX530, but it should share the bugs and limitations as SGX540
  add_definitions(
  -DPANDORA
  -DPowerVR_SGX_540
  -DGLES2
   )
endif(PANDORA)

if(UNIX OR BCMHOST)
  SET( FREETYPE_INCLUDE_DIRS "/usr/include/freetype2/" )
else(UNIX OR BCMHOST)
  SET( ENV{FREETYPE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../freetype )
endif(UNIX OR BCMHOST)
FIND_PACKAGE( Freetype REQUIRED )
include_directories( ${FREETYPE_INCLUDE_DIRS} )

if(VEC4_OPT)
  add_definitions(
    -D__VEC4_OPT
  )
endif(VEC4_OPT)

if(CRC_ARMV8)
  list(REMOVE_ITEM GLideN64_SOURCES
	CRC32.cpp
  )
  list(APPEND GLideN64_SOURCES
	CRC32_ARMV8.cpp
  )
endif(CRC_ARMV8)

if(NEON_OPT)
  add_definitions(
    -D__NEON_OPT
  )
  list(APPEND GLideN64_SOURCES
    3DMathNeon.cpp
    gSPNeon.cpp
  )
  list(REMOVE_ITEM GLideN64_SOURCES
    3DMath.cpp
  )
endif(NEON_OPT)

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

if (USE_UNIFORMBLOCK)
  add_definitions(
	-DGL_USE_UNIFORMBLOCK
   )
  list(APPEND GLideN64_SOURCES
	GLUniforms/UniformBlock.cpp
  )
else (USE_UNIFORMBLOCK)
  list(APPEND GLideN64_SOURCES
	GLUniforms/UniformSet.cpp
  )
endif (USE_UNIFORMBLOCK)

if(GLES2)
  add_definitions(
    -DGLES2
   )
   list(APPEND GLideN64_SOURCES 
    GLES2/GLSLCombiner_gles2.cpp
    BufferCopy/ColorBufferToRDRAMStub.cpp
   )
   SET(OPENGL_LIBRARIES -lGLESv2 -lEGL)
else(GLES2)
  find_package(OpenGL REQUIRED)
  include_directories(${OpenGL_INCLUDE_DIRS})
  link_directories(${OpenGL_LIBRARY_DIRS})
  add_definitions(${OpenGL_DEFINITIONS})
  if(NOT OPENGL_FOUND)
  	message(ERROR " OPENGL not found!")
  endif(NOT OPENGL_FOUND)
  list(APPEND GLideN64_SOURCES 
    OGL3X/GLSLCombiner_ogl3x.cpp
    BufferCopy/ColorBufferToRDRAM_GL.cpp
  )
endif(GLES2)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  #check for G++ 4.8+
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE G++_VERSION)
  if (G++_VERSION VERSION_LESS 4.8)
	message(SEND_ERROR "You need at least G++ 4.8 to compile GLideN64!")
  endif()
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
        if (NOHQ)
	  target_link_libraries(${GLideN64_DLL_NAME} PRIVATE ${OPENGL_LIBRARIES} ${SDL_LIBRARIES} ${FREETYPE_LIBRARIES} osald )
	else (NOHQ)
	  target_link_libraries(${GLideN64_DLL_NAME} PRIVATE ${OPENGL_LIBRARIES} ${SDL_LIBRARIES} ${FREETYPE_LIBRARIES} osald GLideNHQd )
	endif (NOHQ)
  else(SDL)
	if (NOHQ)
	  target_link_libraries(${GLideN64_DLL_NAME} PRIVATE ${OPENGL_LIBRARIES} ${FREETYPE_LIBRARIES} osald )
	else (NOHQ)
	  target_link_libraries(${GLideN64_DLL_NAME} PRIVATE ${OPENGL_LIBRARIES} ${FREETYPE_LIBRARIES} osald GLideNHQd )
	endif (NOHQ)
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
	if (NOHQ)
	  target_link_libraries(${GLideN64_DLL_NAME} ${OPENGL_LIBRARIES} ${SDL_LIBRARIES} ${FREETYPE_LIBRARIES} osal )
	else (NOHQ)
	  target_link_libraries(${GLideN64_DLL_NAME} ${OPENGL_LIBRARIES} ${SDL_LIBRARIES} ${FREETYPE_LIBRARIES} osal GLideNHQ )
	endif (NOHQ)
  else(SDL)
	if (NOHQ)
	  target_link_libraries(${GLideN64_DLL_NAME} ${OPENGL_LIBRARIES} ${FREETYPE_LIBRARIES} osal )
	else (NOHQ)
	  target_link_libraries(${GLideN64_DLL_NAME} ${OPENGL_LIBRARIES} ${FREETYPE_LIBRARIES} osal GLideNHQ )
	endif (NOHQ)
  endif(SDL)
endif( CMAKE_BUILD_TYPE STREQUAL "Release")
