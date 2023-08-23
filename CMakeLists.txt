cmake_minimum_required(VERSION 3.26)
project(raylib_test VERSION 0.1.0)

# Used mostly for convenience things like std::numbers::pi and std::make_array
set(CMAKE_CXX_STANDARD 20)
set(OpenGL_GL_PREFERENCE GLVND)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # clangd wants this, it is not mandatory

add_compile_definitions(-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS)

if (EMSCRIPTEN)
  add_compile_definitions(DASSET_ROOT=\"assets\")
  add_compile_definitions(DPLATFORM_WEB)
  add_definitions(-DPLATFORM_WEB)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY --preload-file ../assets@assets -s TOTAL_MEMORY=134217728")
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
else()
  # TODO: full path in DASSET_ROOT is only required in debug mode
  add_compile_definitions(DNOMINMAX)
  add_compile_definitions(DASSET_ROOT=\"${CMAKE_CURRENT_SOURCE_DIR}/assets\")
endif ()

## Libraries
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

add_subdirectory(${LIB_DIR})

if (MSVC)
    add_compile_options("/W4")
else()
    add_compile_options("-Wall")
endif()

## Executable
# header files aren't required here, but some build systems want to know about them
set(SRC
    component/acceleration.hpp
    component/area_tracker.hpp
    component/behaviour.hpp
    component/camera.hpp
    component/enemy_goal.hpp
    component/enemy_spawn.hpp
    component/health.hpp
    component/max_range.hpp
    component/move_towards.hpp
    component/nav_gate.hpp
    component/projectile.hpp
    component/render.hpp
    component/tile.hpp
    component/transform.hpp
    component/velocity.hpp
    component/walkable.hpp
    external/imgui_internal.hpp
    external/imgui.hpp
    external/imguizmo.hpp
    external/lua.hpp
    external/raygui.cpp
    external/raylib.hpp
    lua_impl/lua_asset_impl.cpp lua_impl/lua_asset_impl.hpp
    lua_impl/lua_entt_impl.cpp lua_impl/lua_entt_impl.hpp
    lua_impl/lua_imgui_impl.cpp lua_impl/lua_imgui_impl.hpp
    lua_impl/lua_imguizmo_impl.cpp lua_impl/lua_imguizmo_impl.hpp
    lua_impl/lua_raylib_impl.cpp lua_impl/lua_raylib_impl.hpp
    lua_impl/lua_register_types.hpp
    lua_impl/lua_register.hpp
    lua_impl/lua_validator.hpp
    lua_impl/lua_world_impl.cpp lua_impl/lua_world_impl.hpp
    system/align_tiles.hpp
    system/area_tracker.hpp
    system/calculate_velocity.hpp
    system/check_health.hpp
    system/max_range.hpp
    system/draw_renderables.hpp
    system/move_entities.hpp
    system/navigate.hpp
    system/update_projectiles.hpp
    entity_reflection/entity_reflection.hpp
    entity_reflection/include_reflection.cpp
    entity_reflection/reflection_area_tracker.hpp
    entity_reflection/reflection_camera.hpp
    entity_reflection/reflection_enemy_goal.hpp
    entity_reflection/reflection_enemy_spawn.hpp
    entity_reflection/reflection_entity.hpp
    entity_reflection/reflection_health.hpp
    entity_reflection/reflection_max_range.hpp
    entity_reflection/reflection_move_towards.hpp
    entity_reflection/reflection_projectile.hpp
    entity_reflection/reflection_render.hpp
    entity_reflection/reflection_transform.hpp
    entity_reflection/reflection_tile.hpp
    entity_reflection/reflection_velocity.hpp
    entity_reflection/reflection_walkable.hpp
    assets.cpp assets.hpp
    imgui_error_check.cpp imgui_error_check.hpp
    main.cpp
    navigation.cpp navigation.hpp
    profiling.cpp profiling.hpp
    raylib_imgui.cpp raylib_imgui.hpp
    world.cpp world.hpp
)
list(TRANSFORM SRC PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/src/)

add_executable(raylib_test
    ${SRC}
)
target_link_libraries(raylib_test PRIVATE
    raylib
    imgui
    imgui_flame_graph
    imguizmo
    lua
)
target_include_directories(raylib_test SYSTEM PRIVATE
    ${LIB_DIR}/entt/src
)
target_include_directories(raylib_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
