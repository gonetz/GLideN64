cmake_minimum_required(VERSION 3.13.4)

project( GLideNHQ )

set(GLideNHQ_SOURCES
  TextureFilters.cpp
  TextureFilters_2xsai.cpp
  TextureFilters_hq2x.cpp
  TextureFilters_hq4x.cpp
  TextureFilters_xbrz.cpp
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

add_library(GLideNHQ STATIC ${GLideNHQ_SOURCES})
set(GLideNHQ_Private_Compile_Definitions TXFILTER_LIB)
set(GLideNHQ_Public_Include_Directories)
set(GLideNHQ_Private_Include_Directories {CMAKE_CURRENT_LIST_DIR}/..)
set(GLideNHQ_Public_Link_Libraries osal)

###########################################
#Platform Specific Configurations
###########################################
if(MSVC)
  list(APPEND GLideNHQ_Private_Compile_Definitions __MSC__)
endif()

if(WIN32)
  list(APPEND GLideNHQ_Private_Compile_Definitions WIN32 OS_WINDOWS _CRT_SECURE_NO_WARNINGS)
endif()

if(PANDORA)
  list(APPEND GLideNHQ_Public_Link_Libraries ${CMAKE_CURRENT_SOURCE_DIR}/lib/libpng.a ${CMAKE_CURRENT_SOURCE_DIR}/lib/libz.a)
endif()

#Mac OSX
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  list(APPEND GLideNHQ_Private_Compile_Definitions OS_MAC_OS_X)
  set(APPLE ON)
#Linux
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  list(APPEND GLideNHQ_Private_Compile_Definitions NDEBUG OS_LINUX)
endif()

if(MINGW OR BCMHOST OR APPLE OR USE_SYSTEM_LIBS)
    FIND_PACKAGE( ZLIB REQUIRED )
    FIND_PACKAGE( PNG REQUIRED )
    list(APPEND GLideNHQ_Public_Link_Libraries ${PNG_LIBRARIES} ${ZLIB_LIBRARIES} dl)
    list(APPEND GLideNHQ_Public_Include_Directories ${PNG_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
endif()

if(ANDROID)
  string(TOLOWER ${CMAKE_BUILD_TYPE} BUILD_VARIANT)
  list(APPEND GLideNHQ_Public_Link_Libraries ${ANDROID_LIB_PATH}/libs/${BUILD_VARIANT}/${ANDROID_ABI}/libpng.a z dl osal)
endif()

###########################################
#Wrap it all together
###########################################
target_compile_definitions(GLideNHQ PRIVATE ${GLideNHQ_Private_Compile_Definitions})
target_include_directories(GLideNHQ PRIVATE ${GLideNHQ_Private_Include_Directories})
target_include_directories(GLideNHQ PUBLIC ${GLideNHQ_Public_Include_Directories})
target_link_libraries(GLideNHQ PUBLIC ${GLideNHQ_Public_Link_Libraries})
set_target_properties(GLideNHQ
                      PROPERTIES
                      DEBUG_POSTFIX d
                      CXX_STANDARD 11
                      COMPILE_DEFINITIONS_DEBUG DEBUG
                      LINK_SEARCH_START_STATIC_DEBUG 1
                      LINK_SEARCH_END_STATIC_DEBUG 1
                      POSITION_INDEPENDENT_CODE ON)
