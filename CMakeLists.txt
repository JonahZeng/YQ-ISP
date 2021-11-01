cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(isp_emulation LANGUAGES CXX C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CONFIGURATION_TYPE Release;Debug)
elseif(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
file(GLOB_RECURSE INC_FILES "${CMAKE_SOURCE_DIR}/inc/*.h")
file(GLOB_RECURSE CALIB_FILES "${CMAKE_SOURCE_DIR}/calib/*.h")
file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE PIPELINE_FILES "${CMAKE_SOURCE_DIR}/pipeline/*.cpp")
file(GLOB_RECURSE CFG_FILES "${CMAKE_SOURCE_DIR}/cfg/*.xml")
#message(STATUS "${INC_FILES} ${SRC_FILES} ${PIPELINE_FILES} ${CFG_FILES} ${CALIB_FILES}")
source_group("header" FILES ${INC_FILES})
source_group("calib" FILES ${CALIB_FILES})
source_group("src" FILES ${SRC_FILES})
source_group("pipeline" FILES ${PIPELINE_FILES})
source_group("cfg" FILES ${CFG_FILES})

find_package(OpenCV REQUIRED)

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

add_executable(isp_emulation ${SRC_FILES} ${INC_FILES} ${PIPELINE_FILES} ${CFG_FILES} ${CALIB_FILES})
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(DNG_SDK_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/include")
    set(DNG_SDK_LIBRARIES_RELEASE "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/dng_sdk_x64_Release.lib")
    set(DNG_SDK_LIBRARIES_DEBUG "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/dng_sdk_x64_Debug.lib")
    set(XMP_LIBRARIES_RELEASE "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/XMPCoreStaticRelease.lib" "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/XMPFilesStaticRelease.lib")
    set(XMP_LIBRARIES_DEBUG "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/XMPCoreStaticDebug.lib" "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/XMPFilesStaticDebug.lib")
    set(JPEG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libjpeg9b/include")
    set(JPEG_LIBRARIES_RELEASE "${CMAKE_SOURCE_DIR}/thirdparty/libjpeg9b/lib/libjpeg.lib")
    set(JPEG_LIBRARIES_DEBUG "${CMAKE_SOURCE_DIR}/thirdparty/libjpeg9b/lib/libjpeg_d.lib")
    set(XML_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/include/libxml2")
	set(XML_BIN_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/bin")
    set(XML_LIBRARIES_RELEASE "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib/libxml2.lib" 
    "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib/libxml2_a.lib" 
    "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib/libxml2_a_dll.lib")
    set(XML_LIBRARIES_DEBUG "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib/libxml2d.lib" 
    "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib/libxml2_ad.lib" 
    "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib/libxml2_a_dlld.lib")
    target_compile_definitions(isp_emulation PUBLIC "qDNGValidate=1;" 
        "qDNGValidateTarget=1;"
        "WIN64;" 
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
        "WIN32;"
        "qWinOS=1;"
        "qMacOS=0;"
        "qVisualC=1;"
        "_WINDOWS=1;"
        "BIB_MULTI_THREAD=1;"
        "_MBCS;"
        "_VC80_UPGRADE=0x0710;")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(DNG_SDK_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/include")
    set(DNG_SDK_LIBRARIES_RELEASE "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/libdng_sdk_release.a")
    set(DNG_SDK_LIBRARIES_DEBUG "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/libdng_sdk_debug.a")
    set(XMP_LIBRARIES_RELEASE "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/staticXMPCoreRelease.ar" "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/staticXMPFilesRelease.ar")
    set(XMP_LIBRARIES_DEBUG "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/staticXMPCoreDebug.ar" "${CMAKE_SOURCE_DIR}/thirdparty/dng_sdk/lib/staticXMPFilesDebug.ar")
    set(JPEG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libjpeg9b/include_linux")
    set(JPEG_LIBRARIES_RELEASE "${CMAKE_SOURCE_DIR}/thirdparty/libjpeg9b/lib_linux/libjpeg.a")
    set(JPEG_LIBRARIES_DEBUG "${CMAKE_SOURCE_DIR}/thirdparty/libjpeg9b/lib_linux/libjpeg.a")
    set(XML_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/include/libxml2_linux")
    set(XML_LIBRARIES_RELEASE "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib_linux/libxml2_release.a" 
    "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib_linux/libxml2mod_release.a")
    set(XML_LIBRARIES_DEBUG "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib_linux/libxml2_debug.a" 
    "${CMAKE_SOURCE_DIR}/thirdparty/libxml2/lib_linux/libxml2mod_debug.a")
    target_compile_definitions(isp_emulation PUBLIC "qDNGValidate=1;"
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
        "qDNG64Bit=1;"
        "qDNGXMPFiles=0;"
        "qDNGXMPDocOps=0;")
endif()

include_directories(${CMAKE_SOURCE_DIR}/inc ${CMAKE_SOURCE_DIR}/calib ${DNG_SDK_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${XML_INCLUDE_DIR} ${OpenCV_INCLUDE_DIRS})
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_link_libraries(isp_emulation 
        optimized ${DNG_SDK_LIBRARIES_RELEASE} ${XMP_LIBRARIES_RELEASE} ${JPEG_LIBRARIES_RELEASE} ${XML_LIBRARIES_RELEASE})
    target_link_libraries(isp_emulation 
        debug ${DNG_SDK_LIBRARIES_DEBUG} ${XMP_LIBRARIES_DEBUG} ${JPEG_LIBRARIES_DEBUG} ${XML_LIBRARIES_DEBUG})

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(isp_emulation 
        optimized ${DNG_SDK_LIBRARIES_RELEASE} ${XMP_LIBRARIES_RELEASE} ${JPEG_LIBRARIES_RELEASE} ${XML_LIBRARIES_RELEASE} z lzma)
    target_link_libraries(isp_emulation 
        debug ${DNG_SDK_LIBRARIES_DEBUG} ${XMP_LIBRARIES_DEBUG} ${JPEG_LIBRARIES_DEBUG} ${XML_LIBRARIES_DEBUG} z lzma)
endif()
target_link_libraries(isp_emulation ${OpenCV_LIBS})

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E 
	copy "${XML_BIN_DIR}/$<$<CONFIG:Release>:libxml2.dll>$<$<CONFIG:Debug>:libxml2d.dll>"  "${EXECUTABLE_OUTPUT_PATH}/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>/libxml2.dll")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
endif()