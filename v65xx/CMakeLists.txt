cmake_minimum_required(VERSION 3.1)
project (v65xx)

set (CMAKE_CXX_STANDARD 11)

add_library (${PROJECT_NAME} SHARED
        src/CMOS65xx.cpp
        )

target_include_directories (${PROJECT_NAME} PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        )

target_link_libraries(${PROJECT_NAME}
        emushared
        )
