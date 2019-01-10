project (emushared)

add_library (${PROJECT_NAME} STATIC
        src/CLog.cpp
        src/CBuffer.cpp
        include/IMemory.h
        )
target_include_directories (${PROJECT_NAME} PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        )
