#pragma once

#include <imgui.h>

// Including imgui's imgui_internal.h creates a bunch of function overloads and
// such - which breaks things - so this local imgui_internal.hpp is used to
// hide the implementation details

bool ValidStackSize(ImGuiContext*);