cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(isp_console LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CONFIGURATION_TYPE Release;Debug)
elseif(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CURRENT_TARGET_NAME isp_console)

file(GLOB SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(${CURRENT_TARGET_NAME} ${SRC_FILES})

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(ISP_LIBRARIES optimized "${LIBRARY_OUTPUT_PATH}/Release/isp_core.lib" debug "${LIBRARY_OUTPUT_PATH}/Debug/isp_core.lib")
    set(DNG_SDK_LIBRARIES optimized "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/dng_sdk_x64_Release.lib" debug "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/dng_sdk_x64_Debug.lib")
    set(XMP_LIBRARIES optimized "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/XMPCoreStaticRelease.lib" debug "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/XMPCoreStaticDebug.lib"
    optimized "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/XMPFilesStaticRelease.lib" debug "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/XMPFilesStaticDebug.lib")
    set(JPEG_LIBRARIES optimized "${CMAKE_SOURCE_DIR}/thirdparty/libjpeg9c/lib/libjpeg.lib" debug "${CMAKE_SOURCE_DIR}/thirdparty/libjpeg9c/lib/libjpeg_d.lib")
    set(XML_BIN_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/bin")
    set(XML_LIBRARIES optimized "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib/libxml2.lib" debug "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib/libxml2d.lib"
    optimized "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib/libxml2_a.lib" debug "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib/libxml2_ad.lib"
    optimized "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib/libxml2_a_dll.lib" debug "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib/libxml2_a_dlld.lib")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(ISP_LIBRARIES optimized "${LIBRARY_OUTPUT_PATH}/isp_core.a" debug "${LIBRARY_OUTPUT_PATH}/isp_core.a")
    set(DNG_SDK_LIBRARIES optimized "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/libdng_sdk_release.a" debug "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/libdng_sdk_debug.a")
    set(XMP_LIBRARIES optimized "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/staticXMPCoreRelease.ar" debug "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/staticXMPCoreDebug.ar"
    optimized "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/staticXMPFilesRelease.ar" debug "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/staticXMPFilesDebug.ar")
    set(JPEG_LIBRARIES optimized "${CMAKE_SOURCE_DIR}/thirdparty/libjpeg9c/lib_linux/libjpeg.a" debug "${CMAKE_SOURCE_DIR}/thirdparty/libjpeg9c/lib_linux/libjpeg_d.a")
    set(XML_LIBRARIES optimized "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib_linux/libxml2_release.a" debug "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib_linux/libxml2_debug.a"
    optimized "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib_linux/libxml2mod_release.a" debug "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib_linux/libxml2mod_debug.a")
endif()

include_directories(${CMAKE_SOURCE_DIR}/inc)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_link_libraries(${CURRENT_TARGET_NAME} ${DNG_SDK_LIBRARIES} ${XMP_LIBRARIES} ${JPEG_LIBRARIES} ${XML_LIBRARIES} ${ISP_LIBRARIES})
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(${CURRENT_TARGET_NAME} ${DNG_SDK_LIBRARIES} ${XMP_LIBRARIES} ${JPEG_LIBRARIES} ${XML_LIBRARIES} ${ISP_LIBRARIES} z lzma dl)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if(EXISTS "${EXECUTABLE_OUTPUT_PATH}/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>/libxml2.dll")
    else()
        add_custom_command(TARGET ${CURRENT_TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E
        copy "${XML_BIN_DIR}/$<$<CONFIG:Release>:libxml2.dll>$<$<CONFIG:Debug>:libxml2d.dll>"  "${EXECUTABLE_OUTPUT_PATH}/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>/libxml2.dll")
    endif()
endif()

