cmake_minimum_required(VERSION 3.11)
project(raylib_test VERSION 0.1.0)

# Used mostly for convenience things like std::numbers::pi and std::make_array
set(CMAKE_CXX_STANDARD 20)
set(OpenGL_GL_PREFERENCE GLVND)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # clangd wants this, it is not mandatory

add_definitions(-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS)

if (EMSCRIPTEN)
  add_compile_definitions(DASSET_ROOT=\"assets\")
  add_compile_definitions(DPLATFORM_WEB)
  add_definitions(-DPLATFORM_WEB)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY --preload-file ../assets@assets -s TOTAL_MEMORY=134217728")
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
else()
  add_compile_definitions(DASSET_ROOT=\"${CMAKE_CURRENT_SOURCE_DIR}/assets\")
endif ()

## Libraries
# Disable warnings for all library source files
# TODO: This doesn't exactly work since the settings aren't restored??
get_directory_property(PROJECT_COMPILE_OPTIONS COMPILE_OPTIONS)
if (MSVC)
    add_compile_options("/w")
else()
    add_compile_options("-Wno-everything")
endif()

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

file(GLOB IMGUI_SRC ${LIB_DIR}/imgui/*.cpp ${LIB_DIR}/imgui/backends/imgui_impl_opengl3.cpp)
file(GLOB LUA_SRC ${LIB_DIR}/lua/*.c)
list(REMOVE_ITEM LUA_SRC ${LIB_DIR}/lua/lua.c ${LIB_DIR}/lua/onelua.c)
add_subdirectory(${LIB_DIR}/raylib)
set(IMGUIZMO_SRC
    ${LIB_DIR}/imguizmo/ImGuizmo.h
    ${LIB_DIR}/imguizmo/ImGuizmo.cpp
)

# Restore warnings and include source files belonging to this project
if (MSVC)
    add_compile_options("/W4")
else()
    add_compile_options("-Wall -Wextra -Wpedantic")
endif()

## Executable
set(SRC
    assets.cpp assets.hpp
    main.cpp
    world.cpp world.hpp
    external/raylib.hpp
    entity/render.hpp
    entity/transform.hpp
    entity/velocity.hpp
    entity_reflection/entity_reflection.hpp
    entity_reflection/reflection_camera.hpp
    entity_reflection/reflection_entity.hpp
    entity_reflection/reflection_render.hpp
    entity_reflection/reflection_transform.hpp
    entity_reflection/reflection_tile.hpp
    entity_reflection/reflection_velocity.hpp
    imgui/imgui_impl.cpp imgui/imgui_impl.hpp
    imgui/imgui_internal.cpp imgui/imgui_internal.hpp
    lua/lua_asset_impl.cpp lua/lua_asset_impl.hpp
    lua/lua_entt_impl.cpp lua/lua_entt_impl.hpp
    lua/lua_imgui_impl.cpp lua/lua_imgui_impl.hpp
    lua/lua_imguizmo_impl.cpp lua/lua_imguizmo_impl.hpp
    lua/lua_raylib_impl.cpp lua/lua_raylib_impl.hpp
    lua/lua_register.hpp
)
list(TRANSFORM SRC PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/src/)

add_executable(raylib_test
    ${SRC}
    ${CUSTOM_IMGUI_SRC}
    ${LUA_SRC}
    ${IMGUI_SRC}
    ${IMGUIZMO_SRC}
)
target_link_libraries(raylib_test PRIVATE
    raylib
)
target_include_directories(raylib_test SYSTEM PRIVATE
    ${LIB_DIR}/raylib/src
    ${LIB_DIR}/lua
    ${LIB_DIR}/imgui
    ${LIB_DIR}/imgui/backends
    ${LIB_DIR}/entt/src
    ${LIB_DIR}/imguizmo
)
target_include_directories(raylib_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)