cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(isp_all LANGUAGES CXX C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)

add_subdirectory(${CMAKE_SOURCE_DIR}/src/console_main)
add_subdirectory(${CMAKE_SOURCE_DIR}/src)