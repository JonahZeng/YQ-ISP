cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(isp_core LANGUAGES CXX)
set(CURRENT_TARGET_NAME isp_core)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CONFIGURATION_TYPE Release;Debug)
elseif(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
file(GLOB_RECURSE INC_FILES "${CMAKE_SOURCE_DIR}/inc/*.h")
file(GLOB_RECURSE CALIB_FILES "${CMAKE_SOURCE_DIR}/calib/*.h")
file(GLOB SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE PIPELINE_FILES "${CMAKE_SOURCE_DIR}/pipeline/*.cpp")
file(GLOB_RECURSE CFG_FILES "${CMAKE_SOURCE_DIR}/cfg/*.xml")
#message(STATUS "${INC_FILES} ${SRC_FILES} ${PIPELINE_FILES} ${CFG_FILES} ${CALIB_FILES}")
source_group("header" FILES ${INC_FILES})
source_group("calib" FILES ${CALIB_FILES})
source_group("src" FILES ${SRC_FILES})
source_group("pipeline" FILES ${PIPELINE_FILES})
source_group("cfg" FILES ${CFG_FILES})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)

add_library(isp_core ${SRC_FILES} ${PIPELINE_FILES} ${CFG_FILES} ${CALIB_FILES} ${INC_FILES})

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(DNG_SDK_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/include")
    set(JPEG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libjpeg9c/include")
    set(XML_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/include/libxml2")
    set(XML_BIN_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/bin")
    target_compile_definitions(${CURRENT_TARGET_NAME} PUBLIC "qDNGValidate=1;"
        "qDNGValidateTarget=1;"
        "$<$<CONFIG:Debug>:"
        "qDNGDebug=1;"
        ">"
        "$<$<CONFIG:Release>:"
        "qDNGDebug=0;"
        ">"
        "$<$<CONFIG:Debug>:"
        "_DEBUG;"
        ">"
        "$<$<CONFIG:Release>:"
        "NDEBUG;"
        ">"
        "_CONSOLE;"
        "_UNICODE;"
        "WIN64;"
        "WIN32;"
        "qiPhone=0;"
        "qAndroid=0;"
        "qWinOS=1;"
        "qMacOS=0;"
        "qVisualC=0;"
        "qWeb=0;"
        "qWinRT=0;"
        "qLinux=0;"
        "_WINDOWS=1;"
        "BIB_MULTI_THREAD=1;"
        "qDNGUseStdInt=1;"
        "qDNGPrintMessages=1;"
        "qDNGPreferGPSMetadataFromXMP=1;"
        "_CRT_SECURE_NO_WARNINGS")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(DNG_SDK_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/include")
    set(JPEG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libjpeg9c/include_linux")
    set(XML_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/include/libxml2_linux")
    target_compile_definitions(${CURRENT_TARGET_NAME} PUBLIC "qDNGValidate=1;"
        "qDNGValidateTarget=1;"
        "$<$<CONFIG:Debug>:"
        "qDNGDebug=1;"
        ">"
        "$<$<CONFIG:Release>:"
        "qDNGDebug=0;"
        ">"
        "$<$<CONFIG:Debug>:"
        "_DEBUG;"
        ">"
        "$<$<CONFIG:Release>:"
        "NDEBUG;"
        ">"
        "_CONSOLE;"
        "_UNICODE;"
        "qiPhone=0;"
        "qAndroid=0;"
        "qWinOS=0;"
        "qMacOS=0;"
        "qVisualC=0;"
        "qWeb=0;"
        "qWinRT=0;"
        "qLinux=1;"
        "_WINDOWS=0;"
        "BIB_MULTI_THREAD=1;"
        "qDNGUseStdInt=1;"
        "qDNGPrintMessages=1;"
        "qDNGPreferGPSMetadataFromXMP=1")
endif()

include_directories(${CMAKE_SOURCE_DIR}/inc ${CMAKE_SOURCE_DIR}/calib ${DNG_SDK_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${XML_INCLUDE_DIR})