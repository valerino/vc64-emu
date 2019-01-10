cmake_minimum_required(VERSION 3.1)
project (vc64-emu)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

add_subdirectory(emushared)
add_subdirectory(v65xx)
add_subdirectory(vc64)
