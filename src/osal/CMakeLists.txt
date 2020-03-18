cmake_minimum_required(VERSION 3.9)
project( osal )

add_library(osal STATIC)
set(osal_Private_Compile_Definitions)
set(osal_Private_Sources)
#########################################################
#Platform Specific Configurations
#########################################################
if(UNIX)
  list(APPEND osal_Private_Sources ${CMAKE_CURRENT_LIST_DIR}/osal_files_unix.c)
  list(APPEND osal_Private_Compile_Definitions NDEBUG OS_LINUX)
endif()

if(WIN32)
  list(APPEND osal_Private_Sources ${CMAKE_CURRENT_LIST_DIR}/osal_files_win32.c)
  list(APPEND osal_Private_Compile_Definitions OS_WINDOWS _CRT_SECURE_NO_WARNINGS)
endif()

if(MSVC)
  list(APPEND osal_Private_Compile_Definitions __MSC__)
endif()

target_sources(osal PRIVATE ${osal_Private_Sources})
target_include_directories(osal PUBLIC ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_directories(osal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set_target_properties(osal
                      PROPERTIES
                      CXX_STANDARD 11
                      DEBUG_POSTFIX d
                      COMPILE_DEFINITIONS_DEBUG DEBUG
                      POSITION_INDEPENDENT_CODE ON)