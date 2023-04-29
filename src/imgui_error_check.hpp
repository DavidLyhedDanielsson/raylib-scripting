#pragma once

// Including imgui's imgui_internal.h creates a bunch of function overloads and
// such - which breaks things like LuaRegister - so this local
// imgui_internal.hpp is used to hide the implementation details

void ErrorCheckEndWindowRecover();
void ErrorCheckEndFrameRecover();