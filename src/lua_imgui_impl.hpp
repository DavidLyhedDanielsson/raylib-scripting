#pragma once

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}
#include <imgui.h>

#include <tuple>
#include <type_traits>

namespace LuaImgui
{
    // https://stackoverflow.com/questions/53945490/how-to-assert-that-a-constexpr-if-else-clause-never-happen
    template<class...>
    constexpr std::false_type always_false{};

    template<typename T>
    auto GetVal(lua_State* lua, int& i)
    {
        // if constexpr seems a bit shorter and surprisingly tidier than
        // template specialization
        static_assert(!std::is_reference_v<T>);

        // The default values here don't always hold up. `0` is an alright
        // choice, but `false` is not that great. It works so far though
        if constexpr(
            std::is_same_v<T, int> || std::is_same_v<T, long> || std::is_same_v<T, unsigned int>)
            return i <= lua_gettop(lua) ? lua_tointeger(lua, i++) : 0;
        else if constexpr(std::is_same_v<T, float>)
            return i <= lua_gettop(lua) ? (float)lua_tonumber(lua, i++) : 0.0f;
        else if constexpr(std::is_same_v<T, const char*>)
            return i <= lua_gettop(lua) ? lua_tostring(lua, i++) : nullptr;
        else if constexpr(std::is_same_v<T, bool>)
            return i <= lua_gettop(lua) ? lua_toboolean(lua, i++) : false;
        else if constexpr(std::is_same_v<T, ImVec2>) // Maybe this should be pushed as an array
        {
            if(i > lua_gettop(lua))
                return ImVec2{0.0f, 0.0f};

            lua_getfield(lua, i, "x");
            float x = lua_tonumber(lua, -1);
            lua_getfield(lua, i, "y");
            float y = lua_tonumber(lua, -1);
            lua_pop(lua, 2);
            i++;
            return ImVec2{x, y};
        }
        else if constexpr(std::is_same_v<T, ImVec4>) // Maybe this should be pushed as an array
        {
            if(i > lua_gettop(lua))
                return ImVec4{0.0f, 0.0f, 0.0f, 0.0f};

            lua_getfield(lua, i, "x");
            float x = lua_tonumber(lua, -1);
            lua_getfield(lua, i, "y");
            float y = lua_tonumber(lua, -1);
            lua_getfield(lua, i, "z");
            float z = lua_tonumber(lua, -1);
            lua_getfield(lua, i, "w");
            float w = lua_tonumber(lua, -1);
            lua_pop(lua, 4);
            i++;
            return ImVec4{x, y, z, w};
        }
        else if constexpr(std::is_same_v<T, float*>)
        {
            auto arr = std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f};
            if(i > lua_gettop(lua))
                return arr;

            if(lua_istable(lua, i))
            {
                int count = luaL_len(lua, i);
                if(count > 4)
                {
                    // Error message at some point
                    return arr;
                }

                for(int j = 1; j <= count; j++)
                {
                    lua_geti(lua, i, j);
                    arr[j - 1] = lua_tonumber(lua, -1);
                    lua_pop(lua, 1);
                }
            }
            else
                arr[0] = lua_tonumber(lua, i);

            i++;

            return arr;
        }
        else if constexpr(std::is_same_v<T, bool*>)
        {
            auto arr = std::array<bool, 4>{false, false, false, false};
            if(i > lua_gettop(lua))
                return arr;

            if(lua_istable(lua, i))
            {
                int count = luaL_len(lua, i);
                if(count > 4)
                {
                    // Error message at some point
                    return arr;
                }

                for(int j = 1; j <= count; j++)
                {
                    lua_geti(lua, i, j);
                    arr[j - 1] = lua_toboolean(lua, -1);
                    lua_pop(lua, 1);
                }
            }
            else
                arr[0] = lua_toboolean(lua, i);

            i++;

            return arr;
        }
        else
            // Clang gives me something like: note:
            // ‘LuaImgui::always_false<ImVec2>’ evaluates to false, meaning
            // ImVec2 is not implemented in the if constexpr chain above
            static_assert(always_false<T>);
    }

    // SetVal should probably also be turned into an if constexpr chain
    template<typename T>
    void SetVal(lua_State* lua, T v)
    {
        if constexpr(std::is_same_v<T, bool>)
            lua_pushboolean(lua, v);
        else if constexpr(std::is_same_v<T, float>)
            lua_pushnumber(lua, v);
        else if constexpr(std::is_same_v<T, ImVec2>)
        {
            lua_createtable(lua, 0, 2);
            lua_pushnumber(lua, v.x);
            lua_setfield(lua, -2, "x");
            lua_pushnumber(lua, v.y);
            lua_setfield(lua, -2, "y");
        }
        else
            static_assert(always_false<T>);
    }

    // Required for functions without parameters (I think) (?)
    std::tuple<> GetVals(lua_State* state, int& index)
    {
        return std::tuple<>();
    }

    template<typename Arg>
    auto GetVals(lua_State* state, int& index)
    {
        return std::make_tuple(GetVal<Arg>(state, index));
    }

    // Use `requires` to disambiguate this from the GetVals above
    template<typename Arg, typename... Args>
    requires(sizeof...(Args) > 0) auto GetVals(lua_State* state, int& index)
    {
        // `current` must be set first to preserve the order of fetching from
        // the stack. Argument order of execution is undefined, so local
        // variables are required
        auto current = std::make_tuple(GetVal<Arg>(state, index));
        auto rest = GetVals<Args...>(state, index);
        return std::tuple_cat(current, rest);
    }

    template<size_t Index, typename... Types, typename... TupTypes>
    requires(Index == sizeof...(Types)) int ReturnVals(
        lua_State* state,
        const std::tuple<TupTypes...>& vals)
    {
        return 0;
    }

    // Does not actually check for a T const*, but for a const T*
    template<typename T>
    bool constexpr is_const_pointer_v = std::is_const_v<std::remove_pointer_t<T>>;

    // Iterates through a tuple and returns values to lua. Docstring will be
    // improved later.
    template<size_t Index, typename... Types, typename... TupTypes>
    requires(Index < sizeof...(Types)) int ReturnVals(
        lua_State* lua,
        const std::tuple<TupTypes...>& vals)
    {
        using T = std::tuple_element_t<Index, std::tuple<Types...>>;
        // using U = std::tuple_element_t<Index, std::tuple<TupTypes...>>;
        if constexpr(!std::is_pointer_v<T> || is_const_pointer_v<T>)
        {
            return 0 + ReturnVals<Index + 1, Types...>(lua, vals);
        }
        else
        {
            using U = std::remove_pointer_t<std::remove_reference_t<T>>;
            int retLength = 0;
            if constexpr(std::is_same_v<U, float>)
            {
                if(lua_istable(lua, Index + 1))
                {
                    int len = luaL_len(lua, Index + 1);
                    lua_createtable(lua, len, 0);
                    for(int i = 0; i < len; ++i)
                    {
                        lua_pushnumber(lua, i + 1);
                        auto val = std::get<Index>(vals).at(i);
                        lua_pushnumber(lua, val);
                        lua_settable(lua, -3);
                    }
                }
                else
                {
                    lua_pushnumber(lua, std::get<Index>(vals).at(0));
                }
                retLength = 1;
            }
            else if constexpr(std::is_same_v<U, bool>)
            {
                if(lua_istable(lua, Index + 1))
                {
                    int len = luaL_len(lua, Index + 1);
                    lua_createtable(lua, len, 0);
                    for(int i = 0; i < len; ++i)
                    {
                        lua_pushnumber(lua, i + 1);
                        auto val = std::get<Index>(vals).at(i);
                        lua_pushboolean(lua, val);
                        lua_settable(lua, -3);
                    }
                }
                else
                {
                    lua_pushboolean(lua, std::get<Index>(vals).at(0));
                }
                retLength = 1;
            }
            else
                static_assert(always_false<T>);

            return retLength + ReturnVals<Index + 1, Types...>(lua, vals);
        }
    }

    // Converts from a tuple that owns all its elements to a tuple where some
    // elements might be pointers. For instance from
    // std::tuple<int, float, const char*> tup
    // to
    // <int, float* const char*>
    // where the float* will be a pointer to std::get<1>(tup)
    template<size_t Index, typename... Types, typename... TupTypes>
    auto Convert(const std::tuple<TupTypes...>& tup)
    {
        using T = std::tuple_element_t<Index, std::tuple<Types...>>;
        using U = std::tuple_element_t<Index, std::tuple<TupTypes...>>;
        if constexpr(std::is_pointer_v<T> && !is_const_pointer_v<T>)
        {
            auto lhs = std::make_tuple((T)&std::get<Index>(tup));
            if constexpr(Index + 1 == sizeof...(Types))
            {
                return lhs;
            }
            else
            {
                auto rhs = Convert<Index + 1, Types...>(tup);
                return std::tuple_cat(lhs, rhs);
            }
        }
        else
        {
            auto lhs = std::make_tuple(std::get<Index>(tup));
            if constexpr(Index + 1 == sizeof...(Types))
            {
                return lhs;
            }
            else
            {
                auto rhs = Convert<Index + 1, Types...>(tup);
                return std::tuple_cat(lhs, rhs);
            }
        }
    }

    template<typename R>
    int LuaWrapper(lua_State* lua)
    {
        int retCount = 0;
        auto f = (R(*)(void))lua_touserdata(lua, lua_upvalueindex(1));
        if constexpr(!std::is_same_v<R, void>)
        {
            R res = f();
            SetVal<R>(lua, res);
            retCount++;
        }
        else
        {
            f();
        }

        return retCount;
    }

    // A lua function which takes a function pointer R (*)(Args) as an upvalue,
    // using R and the Args it is possible to fetch the correct values from the
    // lua stack and call the function pointer stored in upvalue index 1
    template<typename R, typename... Args>
    requires(sizeof...(Args) > 0) int LuaWrapper(lua_State* lua)
    {
        // `arg` needs to be a local variable so it can be passed to GetVals -
        // which takes a reference
        // Keeps track of which stack index to fetch the next parameter from
        int arg = 1;
        // If a function takes a constant reference one of the types in Args
        // will be a const reference, but the tuple must own the value so the
        // parameter can be passed as a reference
        auto tup = GetVals<std::remove_cvref_t<Args>...>(lua, arg);
        // std::apply will not automatically convert owned types to pointers, so
        // a function that takes a float* must be called with a
        // `std::tuple<float*>` and not `std::tuple<float>`. Convert performs
        // this conversion. Any pointers in `tup2` (name will be changed) will
        // point to members in `tup`, so `tup` needs to stick around
        auto tup2 = Convert<0, Args...>(tup);

        int retCount = 0;
        // Call supplied function, since the types R and Args are known the cast
        // is safe.
        // Here it is assumed that the function will return something, which is
        // true for most ImGui functions, but not all. This is a WIP though
        auto f = (R(*)(Args...))lua_touserdata(lua, lua_upvalueindex(1));
        if constexpr(!std::is_same_v<R, void>)
        {
            R res = std::apply(f, tup2);
            SetVal<R>(lua, res);
            retCount++;
        }
        else
        {
            std::apply(f, tup2);
        }

        retCount += ReturnVals<0, Args...>(lua, tup);

        // It is also assumed only one return value is required, which also
        // doesn't really hold up since some functions use pointers to "return"
        // values
        return retCount;
    }

    // Register any function with lua without having to manually manipulate the
    // stack to fetch parameters.
    template<typename R, typename... Args>
    void Register(lua_State* lua, const char* name, R (*f)(Args...))
    {
        // R and Args are known, so upvalues are a perfect place to store the
        // function pointer
        lua_pushlightuserdata(lua, (void*)f);
        lua_pushcclosure(lua, LuaWrapper<R, Args...>, 1);
        lua_setglobal(lua, name);
    }

    void register_imgui(lua_State* lua)
    {
#define QuickRegisterImgui(X) Register(lua, #X, ImGui::X)

        QuickRegisterImgui(Begin);
        QuickRegisterImgui(End);

        Register(
            lua,
            "BeginChild",
            static_cast<bool (*)(const char*, const ImVec2&, bool, ImGuiWindowFlags)>(
                ImGui::BeginChild));
        Register(
            lua,
            "BeginChildId",
            static_cast<bool (*)(ImGuiID, const ImVec2&, bool, ImGuiWindowFlags)>(
                ImGui::BeginChild));
        QuickRegisterImgui(EndChild);

        QuickRegisterImgui(IsWindowAppearing);
        QuickRegisterImgui(IsWindowCollapsed);
        QuickRegisterImgui(IsWindowFocused);
        QuickRegisterImgui(IsWindowHovered);
        // GetWindowDrawList
        QuickRegisterImgui(GetWindowDpiScale);
        QuickRegisterImgui(GetWindowPos);
        QuickRegisterImgui(GetWindowSize);
        QuickRegisterImgui(GetWindowWidth);
        QuickRegisterImgui(GetWindowHeight);
        // GetWindowViewport

        QuickRegisterImgui(SetNextWindowPos);
        QuickRegisterImgui(SetNextWindowSize);
        // SetNextWindowSizeConstraints
        QuickRegisterImgui(SetNextWindowContentSize);
        QuickRegisterImgui(SetNextWindowCollapsed);
        QuickRegisterImgui(SetNextWindowFocus);
        QuickRegisterImgui(SetNextWindowBgAlpha);
        // SetNextWindowViewport
        // SetWindowPos
        // SetWindowSize
        // SetWindowCollapsed
        // SetWindowFocus
        // SetWindowFontScale
        Register(
            lua,
            "SetWindowPos",
            static_cast<void (*)(const char*, const ImVec2&, ImGuiCond)>(ImGui::SetWindowPos));
        Register(
            lua,
            "SetWindowSize",
            static_cast<void (*)(const char*, const ImVec2&, ImGuiCond)>(ImGui::SetWindowSize));
        Register(
            lua,
            "SetWindowCollapsed",
            static_cast<void (*)(const char*, bool, ImGuiCond)>(ImGui::SetWindowCollapsed));
        Register(lua, "SetWindowFocus", static_cast<void (*)(const char*)>(ImGui::SetWindowFocus));

        QuickRegisterImgui(GetContentRegionAvail);
        QuickRegisterImgui(GetContentRegionMax);
        QuickRegisterImgui(GetWindowContentRegionMin);
        QuickRegisterImgui(GetWindowContentRegionMax);

        QuickRegisterImgui(GetScrollX);
        QuickRegisterImgui(GetScrollY);
        QuickRegisterImgui(SetScrollX);
        QuickRegisterImgui(SetScrollY);
        QuickRegisterImgui(GetScrollMaxX);
        QuickRegisterImgui(GetScrollMaxY);
        QuickRegisterImgui(SetScrollHereX);
        QuickRegisterImgui(SetScrollHereY);
        QuickRegisterImgui(SetScrollFromPosX);
        QuickRegisterImgui(SetScrollFromPosY);

        // PushFont(ImFont * font);
        // PopFont();
        Register(
            lua,
            "PushStyleColor",
            static_cast<void (*)(ImGuiCol, ImU32)>(ImGui::PushStyleColor));
        Register(
            lua,
            "PushStyleColor4",
            static_cast<void (*)(ImGuiCol, const ImVec4&)>(ImGui::PushStyleColor));
        QuickRegisterImgui(PopStyleColor);
        Register(
            lua,
            "PushStyleVar",
            static_cast<void (*)(ImGuiStyleVar, float)>(ImGui::PushStyleVar));
        Register(
            lua,
            "PushStyleVar2",
            static_cast<void (*)(ImGuiStyleVar, const ImVec2& val)>(ImGui::PushStyleVar));
        QuickRegisterImgui(PopStyleVar);
        QuickRegisterImgui(PushAllowKeyboardFocus);
        QuickRegisterImgui(PopAllowKeyboardFocus);
        QuickRegisterImgui(PushButtonRepeat);
        QuickRegisterImgui(PopButtonRepeat);

        QuickRegisterImgui(PushItemWidth);
        QuickRegisterImgui(PopItemWidth);
        QuickRegisterImgui(SetNextItemWidth);
        QuickRegisterImgui(CalcItemWidth);
        QuickRegisterImgui(PushTextWrapPos);
        QuickRegisterImgui(PopTextWrapPos);

        QuickRegisterImgui(Button);
        QuickRegisterImgui(SmallButton);
        QuickRegisterImgui(ArrowButton);
        QuickRegisterImgui(BeginCombo);
        QuickRegisterImgui(EndCombo);
        QuickRegisterImgui(DragFloat);
        QuickRegisterImgui(DragFloat2);
        QuickRegisterImgui(DragFloat3);
        QuickRegisterImgui(DragFloat4);
    }
}