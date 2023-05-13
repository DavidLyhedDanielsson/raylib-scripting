cmake_minimum_required(VERSION 3.26)

function(set_compile_options target)
    if (MSVC)
        target_compile_options(${target} PRIVATE "/W4")
    else()
        target_compile_options(${target} PRIVATE "-Wno-everything")
    endif()
endfunction()

file(GLOB IMGUI_SRC
    ${LIB_DIR}/imgui/*.cpp
    ${LIB_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC ${IMGUI_SRC})
target_include_directories(imgui PUBLIC
    ${LIB_DIR}/imgui
    ${LIB_DIR}/imgui/backends
)
set_compile_options(imgui)

file(GLOB IMGUI_FLAME_GRAPH_SRC
    ${LIB_DIR}/imgui-flame-graph/imgui_widget_flamegraph.cpp
    ${LIB_DIR}/imgui-flame-graph/imgui_widget_flamegraph.h
)
add_library(imgui_flame_graph STATIC ${IMGUI_FLAME_GRAPH_SRC})
target_include_directories(imgui_flame_graph PUBLIC
    ${LIB_DIR}/imgui
    ${LIB_DIR}/imgui/backends
    ${LIB_DIR}/imgui-flame-graph
)
target_compile_definitions(imgui_flame_graph PRIVATE IMGUI_DEFINE_MATH_OPERATORS)
set_compile_options(imgui_flame_graph)

set(IMGUIZMO_SRC
    ${LIB_DIR}/imguizmo/ImGuizmo.h
    ${LIB_DIR}/imguizmo/ImGuizmo.cpp
)
add_library(imguizmo STATIC ${IMGUIZMO_SRC})
target_include_directories(imguizmo PUBLIC
    ${LIB_DIR}/imgui
)
set_compile_options(imguizmo)

file(GLOB LUA_SRC
    ${LIB_DIR}/lua/*.c
)
list(REMOVE_ITEM LUA_SRC 
    ${LIB_DIR}/lua/lua.c
    ${LIB_DIR}/lua/onelua.c
)
add_library(lua STATIC ${LUA_SRC})
set_compile_options(lua)

add_subdirectory(${LIB_DIR}/raylib)
set_compile_options(raylib)
