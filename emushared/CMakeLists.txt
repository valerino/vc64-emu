project (emushared)

add_library (${PROJECT_NAME} STATIC
        src/CLog.cpp
        src/CBuffer.cpp
        src/CSDLUtils.cpp
        include/IMemory.h
        )
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# needs sdsl2
find_package(SDL2 REQUIRED)
include_directories(.
        ${SDL2_INCLUDE_DIRS})

target_include_directories (${PROJECT_NAME} PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        )
