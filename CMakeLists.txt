cmake_minimum_required(VERSION 3.1)
project (vc64-emu)

set (CMAKE_CXX_STANDARD 11)

# enable to detect overruns
#set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
#set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

add_executable(${PROJECT_NAME}
        main.cpp
        CDisplay.cpp
        CInput.cpp
        CAudio.cpp
        CMemory.cpp
        CCIABase.cpp
        CCIA1.cpp
        CCIA2.cpp
        CVICII.cpp
        CPLA.cpp
        CSID.cpp
)

# needs sdsl2
find_package(SDL2 REQUIRED)

# needs emushared and v65xx
find_library(LIBEMUSHARED emushared
        PATHS emushared/build)
find_library(LIBV65XX v65xx
        PATHS v65xx/build)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${SDL2_INCLUDE_DIRS}
        emushared
        v65xx
)

target_link_libraries(${PROJECT_NAME}
        ${SDL2_LIBRARIES}
        ${LIBEMUSHARED}
        ${LIBV65XX}
)

# copy the bios folder to build directory
file(COPY bios DESTINATION ${CMAKE_BINARY_DIR})

# also the cpu test binaries
file(COPY v65xx/test DESTINATION ${CMAKE_BINARY_DIR})