cmake_minimum_required(VERSION 3.1)
project (vc64-emu)

set (CMAKE_CXX_STANDARD 11)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# enable to detect overruns
#set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
#set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

add_subdirectory(emushared)
add_subdirectory(v65xx)

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
        CSID.cpp)

# needs sdsl2
find_package(SDL2 REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE
        ${SDL2_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
        ${SDL2_LIBRARIES}
        emushared
        v65xx)

# copy the bios folder to build directory
file(COPY bios DESTINATION ${CMAKE_BINARY_DIR})

# also the cpu test binaries
file(COPY test DESTINATION ${CMAKE_BINARY_DIR})