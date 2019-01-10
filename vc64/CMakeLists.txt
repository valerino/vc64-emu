project(vc64)

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/CDisplay.cpp
        src/CMemory.cpp
        )

# needs sdsl2
find_package(SDL2 REQUIRED)
include_directories(.
        ${SDL2_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
        ${SDL2_LIBRARIES}
        emushared
        v65xx
        )
