cmake_minimum_required(VERSION 2.6)

option(EGL "Set to ON if targeting an EGL device" ${EGL})
option(PANDORA "Set to ON if targeting an OpenPandora" ${PANDORA})
option(MUPENPLUSAPI "Set to ON for Mupen64Plus plugin" ${MUPENPLUSAPI})

project( GLideN64 )

set(GLideN64_SOURCES
  3DMath.cpp
  Combiner.cpp
  CombinerKey.cpp
  CommonPluginAPI.cpp
  Config.cpp
  convert.cpp
  CRC32.cpp
  DepthBuffer.cpp
  DisplayWindow.cpp
  F3D.cpp
  F3DBETA.cpp
  F3DDKR.cpp
  F3DEX.cpp
  F3DEX2.cpp
  F3DEX2CBFD.cpp
  F3DEX2MM.cpp
  F3DGOLDEN.cpp
  F3DTEXA.cpp
  F3DPD.cpp
  F3DSETA.cpp
  FrameBuffer.cpp
  FrameBufferInfo.cpp
  GBI.cpp
  gDP.cpp
  GLideN64.cpp
  GraphicsDrawer.cpp
  gSP.cpp
  Keys.cpp
  L3D.cpp
  L3DEX2.cpp
  L3DEX.cpp
  Log.cpp
  N64.cpp
  NoiseTexture.cpp
  PaletteTexture.cpp
  Performance.cpp
  PostProcessor.cpp
  RDP.cpp
  RSP.cpp
  S2DEX2.cpp
  S2DEX.cpp
  SoftwareRender.cpp
  TexrectDrawer.cpp
  TextDrawer.cpp
  TextureFilterHandler.cpp
  Textures.cpp
  Turbo3D.cpp
  VI.cpp
  ZlutTexture.cpp
  ZSort.cpp
  BufferCopy/ColorBufferToRDRAM.cpp
  BufferCopy/DepthBufferToRDRAM.cpp
  BufferCopy/RDRAMtoColorBuffer.cpp
  DepthBufferRender/ClipPolygon.cpp
  DepthBufferRender/DepthBufferRender.cpp
  common/CommonAPIImpl_common.cpp
  Graphics/Context.cpp
  Graphics/CombinerProgram.cpp
  Graphics/ObjectHandle.cpp
  Graphics/OpenGLContext/GLFunctions.cpp
  Graphics/OpenGLContext/opengl_Attributes.cpp
  Graphics/OpenGLContext/opengl_BufferedDrawer.cpp
  Graphics/OpenGLContext/opengl_BufferManipulationObjectFactory.cpp
  Graphics/OpenGLContext/opengl_CachedFunctions.cpp
  Graphics/OpenGLContext/opengl_ColorBufferReaderWithBufferStorage.cpp
  Graphics/OpenGLContext/opengl_ColorBufferReaderWithPixelBuffer.cpp
  Graphics/OpenGLContext/opengl_ColorBufferReaderWithReadPixels.cpp
  Graphics/OpenGLContext/opengl_ContextImpl.cpp
  Graphics/OpenGLContext/opengl_GLInfo.cpp
  Graphics/OpenGLContext/opengl_Parameters.cpp
  Graphics/OpenGLContext/opengl_TextureManipulationObjectFactory.cpp
  Graphics/OpenGLContext/opengl_UnbufferedDrawer.cpp
  Graphics/OpenGLContext/opengl_Utils.cpp
  Graphics/OpenGLContext/GLSL/glsl_CombinerInputs.cpp
  Graphics/OpenGLContext/GLSL/glsl_CombinerProgramBuilder.cpp
  Graphics/OpenGLContext/GLSL/glsl_CombinerProgramImpl.cpp
  Graphics/OpenGLContext/GLSL/glsl_CombinerProgramUniformFactory.cpp
  Graphics/OpenGLContext/GLSL/glsl_ShaderStorage.cpp
  Graphics/OpenGLContext/GLSL/glsl_SpecialShadersFactory.cpp
  Graphics/OpenGLContext/GLSL/glsl_Utils.cpp
)

#check if we're running on Raspberry Pi
if(EXISTS "/opt/vc/include/bcm_host.h")
  message("bcm_host.h found")
  set(BCMHOST ON)
  set(EGL ON)
  add_definitions(
    -DVC
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
	Graphics/OpenGLContext/mupen64plus/mupen64plus_DisplayWindow.cpp
  )

  set(GLideN64_SOURCES_WIN ${GLideN64_SOURCES_UNIX}
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
	windows/ZilmarAPIImpl_windows.cpp
	Graphics/OpenGLContext/windows/windows_DisplayWindow.cpp
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

if(MINGW)
  add_definitions(
    -DMINGW
  )
endif(MINGW)

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
  -DEGL
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
	add_definitions(
	-DGL_DEBUG
	)
endif( CMAKE_BUILD_TYPE STREQUAL "Debug")

if(EGL)
  add_definitions(
    -DEGL
   )
   SET(OPENGL_LIBRARIES -lEGL)
else(EGL)
  find_package(OpenGL REQUIRED)
  include_directories(${OpenGL_INCLUDE_DIRS})
  link_directories(${OpenGL_LIBRARY_DIRS})
  add_definitions(${OpenGL_DEFINITIONS})
  if(NOT OPENGL_FOUND)
  	message(ERROR " OPENGL not found!")
  endif(NOT OPENGL_FOUND)
endif(EGL)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  #check for compiler version
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE G++_VERSION)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND G++_VERSION VERSION_LESS 4.8)
    message("Found compiler version " ${G++_VERSION})
    message(SEND_ERROR "You need at least G++ 4.8 to compile GLideN64!")
  elseif(G++_VERSION VERSION_LESS 3.3)
    message("Found compiler version " ${G++_VERSION})
    message(SEND_ERROR "You need at least Clang 3.3 to compile GLideN64!")
  endif()
#  SET(GCC_CPP11_COMPILE_FLAGS "-std=c++0x -static-libgcc")
  SET(GCC_CPP11_COMPILE_FLAGS "-std=c++0x")
  SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_CPP11_COMPILE_FLAGS}" )
  SET(GCC_STATIC_LINK_FLAGS "-static-libgcc")
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
