/**
 * Imgui setup/teardown that works with raylib using the imgui OGL backend
 *
 * Raylib uses OpenGL and creates the context. Imgui just binds whichever state
 * and resources it needs, runs some OpenGL draw calls, and then resets the OGL
 * state.
 *
 * The only thing Imgui doesn't know about are inputs. The main purpose of this
 * file is to make sure Imgui receives all input events it needs to work.
 *
 * Some code taken from
 * https://github.com/osom8979/tbag/blob/7f35d1c4e704ac316fcdde0cf9d4e41bd7e401dc/libtbag/ray/RayGui.cpp
 * and
 * https://github.com/WEREMSOFT/c99-raylib-cimgui-template/blob/master/src/cimgui_impl_raylib.h
 */

#pragma once

#include <imgui.h>

namespace RaylibImGui
{
    void Init();
    void Deinit();

    void Begin();
    void End();
}