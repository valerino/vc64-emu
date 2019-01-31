cmake_minimum_required(VERSION 3.1)
project (emushared)

set (CMAKE_CXX_STANDARD 11)

add_library (${PROJECT_NAME} STATIC
        src/CLog.cpp
        src/CBuffer.cpp
        src/CSDLUtils.cpp
        include/IMemory.h
        include/bitutils.h)
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# needs sdsl2
find_package(SDL2 REQUIRED)
include_directories(.
        ${SDL2_INCLUDE_DIRS})

target_include_directories (${PROJECT_NAME} PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        )
