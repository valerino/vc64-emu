cmake_minimum_required(VERSION 3.1)
project (emushared)

set (CMAKE_CXX_STANDARD 11)

add_library (${PROJECT_NAME} STATIC
        bitutils.h
        CBuffer.cpp)
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${PROJECT_NAME} PUBLIC
        ${PROJECT_SOURCE_DIR}
)
