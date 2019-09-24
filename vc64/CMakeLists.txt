project(vc64-emu)

add_executable(${PROJECT_NAME}
        main.cpp
        CDisplay.cpp
        CInput.cpp
        CAudio.cpp
        CMemory.cpp
        CCIA1.cpp
        CCIA2.cpp
        CVICII.cpp
        CSID.cpp
        CIACommon.h)

# needs sdsl2
find_package(SDL2 REQUIRED)
include_directories(
        ${SDL2_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
        ${SDL2_LIBRARIES}
        emushared
        v65xx
        )
