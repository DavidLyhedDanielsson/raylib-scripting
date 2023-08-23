#include "profiling.hpp"
#include "imgui.h"

#include <cassert>
#include <chrono>
#include <external/imgui.hpp>

static uint64_t handleCounter = 0;
static bool stop = false;

namespace Profiling
{
    Scoped::Scoped(const char* name)
    {
        this->handle = Profiling::Start(name);
    }

    Scoped::~Scoped()
    {
        Profiling::End(this->handle);
    }

    // Functions
    Scoped Scope(const char* name)
    {
        return Scoped(name);
    }

    ProfileHandle Start(const char* name)
    {
        auto currentSpan = stack.top();

        ProfileHandle handle{.val = handleCounter++};

        auto now = std::chrono::steady_clock::now();
        currentSpan->children.push_back(
            TimeSpan{.name = name, .start = now, .end = now, .handle = handle});
        stack.push(&currentSpan->children.back());

        return handle;
    }

    void End(ProfileHandle handle)
    {
        assert(stack.top()->handle.val == handle.val);
        End();
    }

    void End()
    {
        assert(!stack.empty());

        stack.top()->end = std::chrono::steady_clock::now();
        stack.pop();
    }

    void NewFrame()
    {
        assert(stack.empty());

        handleCounter = 0;

        if(stop)
            frames.erase(frames.end() - 1);
        else
        {
            if(frames.size() > 600)
                frames.erase(frames.begin());
        }

        ProfileHandle handle{.val = handleCounter++};

        // Avoid high_resolution_clock
        auto now = std::chrono::steady_clock::now();
        frames.push_back(TimeSpan{.name = "frame", .start = now, .end = now, .handle = handle});
        stack.push(&frames.back());
    }

    void EndFrame()
    {
        assert(stack.size() == 1);

        // Avoid high_resolution_clock
        auto now = std::chrono::steady_clock::now();
        frames.back().end = now;

        stack.pop();
    }

    void CalcValue(const TimeSpan& span, int& valueCount)
    {
        valueCount++;
        for(const auto& child : span.children)
            CalcValue(child, valueCount);
    }

    struct FlameData
    {
        std::string_view name;
        std::chrono::steady_clock::time_point frameStart;
        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point end;
        int depth;
    };

    void CreateFlameData(
        std::vector<FlameData>& outFlameData,
        const TimeSpan& span,
        const std::chrono::steady_clock::time_point frameStart,
        const int depth = 1)
    {
        outFlameData.push_back(FlameData{
            .name = span.name,
            .frameStart = frameStart,
            .start = span.start,
            .end = span.end,
            .depth = depth,
        });

        for(const auto& child : span.children)
            CreateFlameData(outFlameData, child, frameStart, depth + 1);
    }

    void Draw()
    {
        // Needs one completely finished frame to render
        if(frames.size() <= 1)
            return;

        /*ImGui::Begin("Profiling");
        ImGui::Checkbox("Stop", &stop);

        static std::optional<int> selectedFrame = 0;
        if(stop)
        {
            if(!selectedFrame)
                selectedFrame = frames.size() - 2;

            ImGui::SliderInt("Frame", &selectedFrame.value(), 0, frames.size() - 2);
        }
        else
            selectedFrame.reset();

        std::vector<FlameData> flameData;
        if(selectedFrame)
            CreateFlameData(flameData, frames[*selectedFrame], frames[*selectedFrame].start);
        else
            CreateFlameData(flameData, *(frames.end() - 2), (frames.end() - 2)->start);

        ImGuiWidgetFlameGraph::PlotFlame(
            "Frame",
            +[](float* start,
                float* end,
                ImU8* level,
                const char** caption,
                const void* data,
                int i) {
                const FlameData& flameData = ((FlameData*)data)[i];

                if(start)
                {
                    dmilliseconds startOffset = flameData.start - flameData.frameStart;
                    dmilliseconds endOffset = flameData.end - flameData.frameStart;

                    *start = startOffset.count();
                    *end = endOffset.count();
                }

                if(level)
                    *level = flameData.depth;

                if(caption)
                    *caption = flameData.name.data();
            },
            flameData.data(),
            flameData.size());
        ImGui::End();*/
    }
};