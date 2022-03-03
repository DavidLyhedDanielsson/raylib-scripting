cmake_minimum_required(VERSION 3.11)
project(raylib_test VERSION 0.1.0)

# Used mostly for convenience things like std::numbers::pi and std::make_array
set(CMAKE_CXX_STANDARD 20)
set(OpenGL_GL_PREFERENCE GLVND)

add_definitions(-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS)

if (EMSCRIPTEN)
  add_compile_definitions(DASSET_ROOT=\"assets\")
  add_compile_definitions(DPLATFORM_WEB)
  add_definitions(-DPLATFORM_WEB)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY --preload-file ../assets@assets -s TOTAL_MEMORY=67108864")
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
else()
  add_compile_definitions(DASSET_ROOT=\"${CMAKE_CURRENT_SOURCE_DIR}/assets\")
endif ()

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

file(GLOB SRCS src/*.cpp src/lua/*.cpp src/imgui/*.cpp)
file(GLOB LUA_SRC ${LIB_DIR}/lua/*.c)
list(REMOVE_ITEM LUA_SRC ${LIB_DIR}/lua/lua.c ${LIB_DIR}/lua/onelua.c)
file(GLOB IMGUI_SRC ${LIB_DIR}/imgui/*.cpp ${LIB_DIR}/imgui/backends/imgui_impl_opengl3.cpp)

add_subdirectory(${LIB_DIR}/raylib)
link_libraries(raylib)
include_directories(SYSTEM ${LIB_DIR}/raylib/src ${LIB_DIR}/lua ${LIB_DIR}/imgui ${LIB_DIR}/imgui/backends ${LIB_DIR}/entt/src)

add_executable(raylib_test ${SRCS} ${LUA_SRC} ${IMGUI_SRC})

set_source_files_properties(${SRCS} PROPERTIES COMPILE_FLAGS "-Wall -Wextra -Wpedantic")