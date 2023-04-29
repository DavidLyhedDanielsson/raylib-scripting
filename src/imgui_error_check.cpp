#include "imgui_error_check.hpp"

#include <external/imgui_internal.hpp>
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