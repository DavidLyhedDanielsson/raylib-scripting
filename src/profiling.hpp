#pragma once

#include "imgui_internal.h"
#include <chrono>
#include <ratio>
#include <stack>
#include <string_view>
#include <vector>

#define CONCAT_IMPL(pre, suff) pre##suff
#define CONCAT(pre, suff) CONCAT_IMPL(pre, suff)

#define PROFILE_CALL(func, ...) Profiling::ProfileCall(#func, [&]() { func(__VA_ARGS__); })
#define PROFILE_SCOPE(name) auto CONCAT(profileVar, __LINE__) = Profiling::Scoped(name)

namespace Profiling
{
    struct ProfileHandle
    {
        uint64_t val;
    };

    struct TimeSpan
    {
        // std::string_view name;
        char name[64];
        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point end;
        ProfileHandle handle;

        // TODO: use PMR to avoid overhead
        std::vector<TimeSpan> children;
    };

    static std::vector<TimeSpan> frames;
    static std::stack<TimeSpan*> stack;

    // https://philippegroarke.com/posts/2018/chrono_for_humans/
    using dnanoseconds = std::chrono::duration<double, std::nano>;
    using dmicroseconds = std::chrono::duration<double, std::micro>;
    using dmilliseconds = std::chrono::duration<double, std::milli>;
    using dseconds = std::chrono::duration<double>;
    using dminutes = std::chrono::duration<double, std::ratio<60>>;
    using dhours = std::chrono::duration<double, std::ratio<3600>>;
    using ddays = std::chrono::duration<double, std::ratio<86400>>;
    using dweeks = std::chrono::duration<double, std::ratio<604800>>;
    using dmonths = std::chrono::duration<double, std::ratio<2629746>>;
    using dyears = std::chrono::duration<double, std::ratio<31556952>>;

    class Scoped
    {
        ProfileHandle handle;

      public:
        Scoped(const char* name);
        Scoped(long long number);
        ~Scoped();
    };

    Scoped Scope(const char*);

    ProfileHandle Start(const char*);
    ProfileHandle Start(long long);
    void End(ProfileHandle handle);
    void End(); // Unsafe but nice to use. TODO: Come up with better API

    template<typename T>
    void ProfileCall(const char* name, const T& func)
    {
        auto handle = Profiling::Start(name);
        func();
        Profiling::End(handle);
    }

    void NewFrame();
    void EndFrame();

    void Draw();
};