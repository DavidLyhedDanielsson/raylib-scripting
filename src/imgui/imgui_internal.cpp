#include "imgui_internal.hpp" // The corresponding header for this cpp file
#include <imgui_internal.h> //  The actual imgui_internal header
#include <iostream>

void ImGuiLog(void*, const char* fmt, ...)
{
    // Apparently I'm not cool enough to get va_list to work
    // std::va_list args;
    // va_start(args, fmt);
    fprintf(stderr, "Recovered from ImGui error\n");
    // va_end(args);
}

void ErrorCheckEndWindowRecover()
{
    ImGui::ErrorCheckEndWindowRecover(ImGuiLog);
}
void ErrorCheckEndFrameRecover()
{
    ImGui::ErrorCheckEndFrameRecover(ImGuiLog);
}