project (v65xx)

add_library (${PROJECT_NAME} SHARED
        src/CMOS65xx.cpp
        )

target_include_directories (${PROJECT_NAME} PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        )

target_link_libraries(${PROJECT_NAME}
        emushared
        )
