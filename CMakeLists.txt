cmake_minimum_required(VERSION 3.11)
project(raylib_test VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(OpenGL_GL_PREFERENCE GLVND)

if (EMSCRIPTEN)
  add_compile_definitions(DASSET_ROOT=\"assets\")
  add_compile_definitions(DPLATFORM_WEB)
  add_definitions(-DPLATFORM_WEB)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY --preload-file ../assets@assets -s TOTAL_MEMORY=67108864")
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
else()
  add_compile_definitions(DASSET_ROOT=\"${CMAKE_CURRENT_SOURCE_DIR}/assets\")
endif ()


file(GLOB SRCS *.cpp *.h)

add_subdirectory(raylib)
link_libraries(raylib)
include_directories(./raylib/src)

add_executable(raylib_test ${SRCS})
