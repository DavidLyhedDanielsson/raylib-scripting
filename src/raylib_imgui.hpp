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
 * See .cpp file for implementation comments
 */

#pragma once

#include <external/imgui.hpp>

namespace RaylibImGui
{
    void Init();
    void Deinit();

    void Begin();
    void End();
}