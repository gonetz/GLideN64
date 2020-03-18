cmake_minimum_required(VERSION 3.9)

option(EGL "Set to ON if targeting an EGL device" ${EGL})
option(PANDORA "Set to ON if targeting an OpenPandora" ${PANDORA})
option(ODROID "Set to ON if targeting an Odroid" ${ODROID})
option(MUPENPLUSAPI "Set to ON for Mupen64Plus plugin" ${MUPENPLUSAPI})
option(MESA "Set to ON to disable Raspberry Pi autodetection" ${MESA})
option(VERO4K "Set to ON if targeting a Vero4k" ${VERO4K})
option(ANDROID "Set to ON if targeting an Android device" ${ANDROID})
option(GL_PROFILE "Set to ON to turn on GL profiling" ${GL_PROFILE})

# run script to generate Revision.h
set(PATH_REVISION "Revision.h")
add_custom_command(
        OUTPUT ${PATH_REVISION}
        COMMAND ${CMAKE_SOURCE_DIR}/getRevision.sh
        COMMENT "Generate Git version"
)

project( GLideN64 )

set(GLideN64_SOURCES
  3DMath.cpp
  Combiner.cpp
  CombinerKey.cpp
  CommonPluginAPI.cpp
  Config.cpp
  convert.cpp
  CRC32.cpp
  DebugDump.cpp
  Debugger.cpp
  DepthBuffer.cpp
  DisplayWindow.cpp
  DisplayLoadProgress.cpp
  FrameBuffer.cpp
  FrameBufferInfo.cpp
  GBI.cpp
  gDP.cpp
  GLideN64.cpp
  GraphicsDrawer.cpp
  gSP.cpp
  Keys.cpp
  Log.cpp
  N64.cpp
  NoiseTexture.cpp
  PaletteTexture.cpp
  Performance.cpp
  PostProcessor.cpp
  RDP.cpp
  RSP.cpp
  RSP_LoadMatrix.cpp
  SoftwareRender.cpp
  TexrectDrawer.cpp
  TextDrawer.cpp
  TextureFilterHandler.cpp
  Textures.cpp
  VI.cpp
  ZlutTexture.cpp
  BufferCopy/BlueNoiseTexture.cpp
  BufferCopy/ColorBufferToRDRAM.cpp
  BufferCopy/DepthBufferToRDRAM.cpp
  BufferCopy/RDRAMtoColorBuffer.cpp
  DepthBufferRender/ClipPolygon.cpp
  DepthBufferRender/DepthBufferRender.cpp
  common/CommonAPIImpl_common.cpp
  Graphics/Context.cpp
  Graphics/ColorBufferReader.cpp
  Graphics/CombinerProgram.cpp
  Graphics/ObjectHandle.cpp
  Graphics/OpenGLContext/GLFunctions.cpp
  Graphics/OpenGLContext/ThreadedOpenGl/opengl_Command.cpp
  Graphics/OpenGLContext/ThreadedOpenGl/opengl_ObjectPool.cpp
  Graphics/OpenGLContext/ThreadedOpenGl/opengl_Wrapper.cpp
  Graphics/OpenGLContext/ThreadedOpenGl/opengl_WrappedFunctions.cpp
  Graphics/OpenGLContext/ThreadedOpenGl/RingBufferPool.cpp
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
  Graphics/OpenGLContext/GLSL/glsl_FXAA.cpp
  Graphics/OpenGLContext/GLSL/glsl_ShaderStorage.cpp
  Graphics/OpenGLContext/GLSL/glsl_SpecialShadersFactory.cpp
  Graphics/OpenGLContext/GLSL/glsl_Utils.cpp
  uCodes/F3D.cpp
  uCodes/F3DBETA.cpp
  uCodes/F3DDKR.cpp
  uCodes/F3DEX.cpp
  uCodes/F3DAM.cpp
  uCodes/F3DEX2.cpp
  uCodes/F3DEX2ACCLAIM.cpp
  uCodes/F3DEX2CBFD.cpp
  uCodes/F3DZEX2.cpp
  uCodes/F3DFLX2.cpp
  uCodes/F3DGOLDEN.cpp
  uCodes/F3DTEXA.cpp
  uCodes/F3DPD.cpp
  uCodes/F3DSETA.cpp
  uCodes/F5Indi_Naboo.cpp
  uCodes/F5Rogue.cpp
  uCodes/L3D.cpp
  uCodes/L3DEX2.cpp
  uCodes/L3DEX.cpp
  uCodes/S2DEX2.cpp
  uCodes/S2DEX.cpp
  uCodes/T3DUX.cpp
  uCodes/Turbo3D.cpp
  uCodes/ZSort.cpp
  uCodes/ZSortBOSS.cpp
)

set(GLideN64_SOURCES_UNIX
  MupenPlusPluginAPI.cpp
  mupenplus/Config_mupenplus.cpp
  mupenplus/CommonAPIImpl_mupenplus.cpp
  mupenplus/MemoryStatus_mupenplus.cpp
  mupenplus/MupenPlusAPIImpl.cpp
  Graphics/OpenGLContext/mupen64plus/mupen64plus_DisplayWindow.cpp
  )

set(GLideN64_SOURCES_WIN
  ZilmarPluginAPI.cpp
  windows/Config_windows.cpp
  windows/CommonAPIImpl_windows.cpp
  windows/GLideN64_windows.cpp
  windows/MemoryStatus_windows.cpp
  common/GLFunctions.cpp
  windows/ZilmarAPIImpl_windows.cpp
  Graphics/OpenGLContext/windows/windows_DisplayWindow.cpp
  )

set(GLideN64_SOURCES_ANDROID
   Log_android.cpp
   Graphics/OpenGLContext/opengl_ColorBufferReaderWithEGLImage.cpp
   Graphics/OpenGLContext/GraphicBuffer/PrivateApi/GraphicBuffer.cpp
   Graphics/OpenGLContext/GraphicBuffer/PrivateApi/libhardware.cpp
   Graphics/OpenGLContext/GraphicBuffer/PublicApi/android_hardware_buffer_compat.cpp
   Graphics/OpenGLContext/GraphicBuffer/GraphicBufferWrapper.cpp
  )

#check if we're running on Raspberry Pi
if(EXISTS "${CMAKE_FIND_ROOT_PATH}/opt/vc/include/bcm_host.h" AND NOT MESA)
  message("bcm_host.h found")
  set(BCMHOST ON)
  set(EGL ON)
  set(EGL_LIB -lbrcmEGL -lbrcmGLESv2)
else(EXISTS "${CMAKE_FIND_ROOT_PATH}/opt/vc/include/bcm_host.h" AND NOT MESA)
  set(EGL_LIB -lEGL)
endif(EXISTS "${CMAKE_FIND_ROOT_PATH}/opt/vc/include/bcm_host.h" AND NOT MESA)


add_subdirectory( osal )

if (NOHQ)
  list(APPEND GLideN64_SOURCES TxFilterStub.cpp)
else (NOHQ)
  add_subdirectory( GLideNHQ )
  include_directories( GLideNHQ )
endif(NOHQ)


if(ANDROID)
   set(EGL ON)
   if(ANDROID_ABI STREQUAL "armeabi-v7a" OR ANDROID_ABI STREQUAL "arm64-v8a")
       set(NEON_OPT ON)
       set(VEC4_OPT ON)
       set(CRC_NEON ON)
   elseif(ANDROID_ABI STREQUAL "x86" OR ANDROID_ABI STREQUAL "x86_64")
       set(CRC_OPT ON)
       set(VEC4_OPT ON)
   endif()

   list(APPEND GLideN64_SOURCES ${GLideN64_SOURCES_ANDROID})

   list(REMOVE_ITEM GLideN64_SOURCES
           Log.cpp
           )

   find_library( # Defines the name of the path variable that stores the
           # location of the NDK library.
           LOG_LIB

           # Specifies the name of the NDK library that
           # CMake needs to locate.
           log )
endif(ANDROID)


#########################################################
# Build type
#########################################################
if( NOT CMAKE_BUILD_TYPE)
	set( CMAKE_BUILD_TYPE Release)
endif( NOT CMAKE_BUILD_TYPE)

#########################################################
#Global builds settings
#########################################################
set(CMAKE_CXX_STANDARD 11) #Set the C++ standard
set(CMAKE_CXX_STANDARD_REQUIRED ON) #Require C++ stanard in CMAKE_CXX_STANDARD to generate the project
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(GLideN64 SHARED )

set(GLideN64_Output_Name GLideN64)
set(GlideN64_Private_Compile_Definitions)
set(GlideN64_Private_Include_Directories)
set(GlideN64_Private_Link_Directories)
set(GlideN64_Private_Link_Libraries)
set(GlideN64_Private_Link_Options)


#########################################################
#Platform Specific Configurations
#########################################################
list(APPEND GlideN64_Private_Include_Directories ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/inc)

if(SDL)
  find_package(SDL REQUIRED)
  list(APPEND GlideN64_Private_Include_Directories ${SDL_INCLUDE_DIR})
  list(APPEND GlideN64_Private_Compile_Definitions USE_SDL)
  list(APPEND GlideN64_Private_Link_Libraries ${SDL_LIBRARIES})
endif(SDL)


if(BCMHOST)
  list(APPEND GlideN64_Private_Include_Directories
    "${CMAKE_FIND_ROOT_PATH}/opt/vc/include"
    "${CMAKE_FIND_ROOT_PATH}/opt/vc/include/interface/vcos"
    "${CMAKE_FIND_ROOT_PATH}/opt/vc/include/interface/vmcs_host/linux"
    "${CMAKE_FIND_ROOT_PATH}/opt/vc/include/interface/vcos/pthreads")

  list(APPEND GlideN64_Private_Link_Directories
    "${CMAKE_FIND_ROOT_PATH}/opt/vc/lib"
    "${CMAKE_FIND_ROOT_PATH}/opt/vc/lib/GL")

  list(APPEND GlideN64_Private_Compile_Definitions VC)
endif()

if(MUPENPLUSAPI)
  set(GLideN64_Output_Name mupen64plus-video-GLideN64)
  list(APPEND GlideN64_Private_Compile_Definitions MUPENPLUSAPI TXFILTER_LIB)
  list(APPEND GlideN64_Private_Include_Directories ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/inc)
else()
  if(UNIX)
    message(FATAL_ERROR "UNIX build requires MUPENPLUSAPI!")
  endif()
endif(MUPENPLUSAPI)

####Android Support####
if(ANDROID)
    set(GLideN64_Output_Name libmupen64plus-video-GLideN64)
    list(APPEND GlideN64_Private_Compile_Definitions OS_ANDROID GL_EGLEXT_PROTOTYPES EGL)
    add_library(ANDROID_FREETYPE SHARED IMPORTED)
    target_include_directories(ANDROID_FREETYPE PUBLIC "${ANDROID_LIB_PATH}/freetype/include")
    if( CMAKE_BUILD_TYPE STREQUAL "Debug")
        set( BUILD_VARIANT "debug" )
    else()
        set( BUILD_VARIANT "release" )
    endif()

    set_target_properties( # Specifies the target library.
            ANDROID_FREETYPE

            # Specifies the parameter you want to define.
            PROPERTIES IMPORTED_LOCATION

            # Provides the path to the library you want to import.
            ${ANDROID_LIB_PATH}/libs/${BUILD_VARIANT}/${ANDROID_ABI}/libfreetype.so)
    set(FREETYPE_LIBRARIES ANDROID_FREETYPE)
else()
  find_package(Freetype REQUIRED )
  list(APPEND GlideN64_Private_Link_Libraries ${FREETYPE_LIBRARIES})
  list(APPEND GlideN64_Private_Include_Directories ${FREETYPE_INCLUDE_DIRS})
endif()

####Windows Support####
if(WIN32)
  list(APPEND GLideN64_SOURCES ${GLideN64_SOURCES_WIN})
  list(APPEND GlideN64_Private_Compile_Definitions OS_WINDOWS __WIN32__ WIN32 _WIN32_ASM _CRT_SECURE_NO_WARNINGS __MSC__)
endif(WIN32)

  
#Mac OSX
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  list(APPEND GLideN64_SOURCES ${GLideN64_SOURCES_UNIX})
  list(APPEND GlideN64_Private_Compile_Definitions OS_MAC_OS_X)
  if (NOT CMAKE_VERSION VERSION_LESS 2.8.12)
    set(CMAKE_MACOSX_RPATH 0)
  endif()
#Linux
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  list(APPEND GLideN64_SOURCES ${GLideN64_SOURCES_UNIX})
  list(APPEND GlideN64_Private_Compile_Definitions OS_LINUX)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

#########################################################
#Build Environment Specific Configurations
#########################################################
if(MINGW)
  list(APPEND GlideN64_Private_Compile_Definitions MINGW)
endif()

if(CRC_ARMV8)
  list(REMOVE_ITEM GLideN64_SOURCES CRC32.cpp )
  list(APPEND GLideN64_SOURCES CRC32_ARMV8.cpp )
elseif(CRC_OPT)
  list(REMOVE_ITEM GLideN64_SOURCES CRC32.cpp)
  list(APPEND GLideN64_SOURCES CRC_OPT.cpp xxHash/xxhash.c)
elseif(CRC_NEON)
  list(REMOVE_ITEM GLideN64_SOURCES CRC32.cpp )
  list(APPEND GLideN64_SOURCES Neon/CRC_OPT_NEON.cpp xxHash/xxhash.c)
endif()

if(NEON_OPT)
  list(APPEND GlideN64_Private_Compile_Definitions __NEON_OPT)

  list(APPEND GLideN64_SOURCES
    Neon/3DMathNeon.cpp
    Neon/gSPNeon.cpp
    Neon/RSP_LoadMatrixNeon.cpp
  )
  list(REMOVE_ITEM GLideN64_SOURCES
    3DMath.cpp
    RSP_LoadMatrix.cpp
  )
endif()

#########################################################
#Find and configure packages
#########################################################
if(EGL)
  add_definitions(-DEGL)
  set(OPENGL_LIBRARIES ${EGL_LIB})
else(EGL)
  cmake_policy(SET CMP0072 NEW)
  find_package(OpenGL REQUIRED)
  list(APPEND GlideN64_Private_Include_Directories ${OpenGL_INCLUDE_DIRS})
  list(APPEND GlideN64_Private_Link_Directories ${OpenGL_LIBRARY_DIRS})
  list(APPEND GlideN64_Private_Compile_Definitions ${OpenGL_DEFINITIONS})
endif(EGL)

#########################################################
#Include Directories
#########################################################


#########################################################
#Compile definitions
#########################################################

#Pandora as a SGX530, but it should share the bugs and limitations as SGX540
if(PANDORA)
  list(APPEND GlideN64_Private_Compile_Definitions PANDORA PowerVR_SGX_540 EGL)
endif()

if(GL_PROFILE)
    list(APPEND GlideN64_Private_Compile_Definitions GL_PROFILE GL_DEBUG)
endif()

if(ODROID)
  list(APPEND GlideN64_Private_Compile_Definitions ODROID)
endif()

if(VEC4_OPT)
  list(APPEND GlideN64_Private_Compile_Definitions __VEC4_OPT)
endif()

if(VERO4K)
  list(APPEND GlideN64_Private_Compile_Definitions VERO4K)
endif()


if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT GL_PROFILE)
  list(APPEND GlideN64_Private_Compile_Definitions GL_DEBUG)
endif()

#########################################################
#Link Directories
#########################################################

#########################################################
#Linker flags
#########################################################
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    list(APPEND GlideN64_Private_Link_Options "-static-libgcc")
endif()

#########################################################
#Generate the list of libraries that we need to link with
#########################################################
list(APPEND GlideN64_Private_Link_Libraries ${OPENGL_LIBRARIES} ${FREETYPE_LIBRARIES} ${LOG_LIB} osal GLideNHQ)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  message(FATAL_ERROR "Invalid Configraion set ${CMAKE_BUILD_TYPE}")
endif()

#########################################################
#Wrap it all together
#########################################################
target_sources(GLideN64 PRIVATE ${GLideN64_SOURCES} ${PATH_REVISION})
target_compile_definitions(GLideN64 PRIVATE ${GlideN64_Private_Compile_Definitions})
target_link_directories(GLideN64 PRIVATE ${GlideN64_Private_Link_Directories})
target_link_options(GLideN64 PRIVATE ${GlideN64_Private_Link_Options})
target_link_libraries(GLideN64 PRIVATE ${GlideN64_Private_Link_Libraries})
target_include_directories(GLideN64 PRIVATE ${GlideN64_Private_Include_Directories})
set_target_properties(GLideN64
                      PROPERTIES
                      PREFIX ""
                      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/plugin/${CMAKE_BUILD_TYPE}
                      OUTPUT_NAME ${GLideN64_Output_Name})
