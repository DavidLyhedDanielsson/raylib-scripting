cmake_minimum_required(VERSION 3.0.0)
project(raylib_test VERSION 0.1.0)

file(GLOB SRCS *.cpp *.h)

add_compile_definitions(DASSET_ROOT=\"${CMAKE_CURRENT_SOURCE_DIR}/assets\")

link_libraries(raylib)

add_executable(raylib_test ${SRCS})
