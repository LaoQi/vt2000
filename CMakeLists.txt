cmake_minimum_required(VERSION 3.22)
project(vt2000 C)

set(CMAKE_C_STANDARD 99)

include_directories(
        "${PROJECT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/src"
)

#file(GLOB VT2000_SRC
#        "${PROJECT_SOURCE_DIR}/src/*.c")

file(GLOB VT2000_SRC
        "${PROJECT_SOURCE_DIR}/src/font.c")

IF(WIN32)

    add_definitions(-DD2D_USE_C_DEFINITIONS)

    add_executable(vt2000 ${VT2000_SRC} test_font.c)
    target_link_libraries(vt2000 gdi32 Msimg32)
ENDIF(WIN32)

