project(vc64)

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/CDisplay.cpp
        src/CInput.cpp
        src/CAudio.cpp
        src/CMemory.cpp
        src/CCIA1.cpp
        src/CCIA2.cpp
        src/CVICII.cpp
        src/CSID.cpp
        src/CIACommon.h)

# needs sdsl2
find_package(SDL2 REQUIRED)
include_directories(.
        ${SDL2_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
        ${SDL2_LIBRARIES}
        emushared
        v65xx
        )
