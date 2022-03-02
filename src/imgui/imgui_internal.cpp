#include "imgui_internal.h" // The corresponding header for this cpp file
#include <imgui_internal.h> //  The actual imgui_internal header

bool ValidStackSize(ImGuiContext* context)
{
    return context->CurrentWindowStack.Size == 1;
}