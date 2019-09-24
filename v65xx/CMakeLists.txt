cmake_minimum_required(VERSION 3.1)
project (v65xx)

set (CMAKE_CXX_STANDARD 11)

add_library (${PROJECT_NAME} SHARED
        CMOS65xx.cpp
        )

# needs sdsl2
find_package(SDL2 REQUIRED)
target_include_directories (${PROJECT_NAME} PUBLIC
        ${PROJECT_SOURCE_DIR}
        PRIVATE
        ${SDL2_INCLUDE_DIRS})
        
target_link_libraries(${PROJECT_NAME}
        ${SDL2_LIBRARIES}
        emushared)
