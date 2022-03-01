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
    constexpr int MAX_VARIADIC_ARG_COUNT = 32;
    constexpr int STRING_BUFFER_LENGTH = 256;

    template<typename T>
    struct Variadic
    {
        using value_type = T;
        std::array<T, MAX_VARIADIC_ARG_COUNT> arr;
        int count;

        // strcpy_s isn't a requirement in the standard and sprintf has at least
        // some overhead. Fingers crossed that the compiler will optimize this
        // to at least sprintf performance
        inline std::array<char, STRING_BUFFER_LENGTH> strcpy()
        {
            std::array<char, STRING_BUFFER_LENGTH> buffer;
            int writtenCharacters = 0;
            for(int i = 0; i < count; ++i)
            {
                for(int j = 0; writtenCharacters < STRING_BUFFER_LENGTH - 1;
                    ++writtenCharacters, ++j)
                {
                    if(arr[i][j] == '\0')
                        break;
                    buffer[writtenCharacters] = arr[i][j];
                }

                if(writtenCharacters == STRING_BUFFER_LENGTH - 1)
                    break;
            }
            buffer[writtenCharacters] = '\0';
            return buffer;
        }
    };

    template<typename>
    struct is_variadic: std::false_type
    {
    };

    template<typename T>
    struct is_variadic<Variadic<T>>: std::true_type
    {
    };

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
        else if constexpr(std::is_same_v<T, float*> || std::is_same_v<T, const float*>)
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
        else if constexpr(std::is_same_v<T, int*>)
        {
            auto arr = std::array<int, 4>{0, 0, 0, 0};
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
                    arr[j - 1] = lua_tointeger(lua, -1);
                    lua_pop(lua, 1);
                }
            }
            else
                arr[0] = lua_tointeger(lua, i);

            i++;

            return arr;
        }
        else if constexpr(is_variadic<T>::value)
        {
            auto var = Variadic<typename T::value_type>();
            // Eat the rest of the arguments. Nom nom nom
            var.count = std::min(lua_gettop(lua) - i + 1, MAX_VARIADIC_ARG_COUNT);
            for(int j = 0; i <= lua_gettop(lua); ++i, ++j)
                var.arr.at(j) = lua_tostring(lua, i);
            return var;
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
        else if constexpr(std::is_same_v<T, int>)
            lua_pushinteger(lua, v);
        else if constexpr(std::is_same_v<T, float>)
            lua_pushnumber(lua, v);
        else if constexpr(std::is_same_v<T, unsigned int>)
            lua_pushinteger(lua, v);
        else if constexpr(std::is_same_v<T, const char*>)
            lua_pushstring(lua, v);
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
            else if constexpr(std::is_same_v<U, int>)
            {
                if(lua_istable(lua, Index + 1))
                {
                    int len = luaL_len(lua, Index + 1);
                    lua_createtable(lua, len, 0);
                    for(int i = 0; i < len; ++i)
                    {
                        lua_pushnumber(lua, i + 1);
                        auto val = std::get<Index>(vals).at(i);
                        lua_pushinteger(lua, val);
                        lua_settable(lua, -3);
                    }
                }
                else
                {
                    lua_pushinteger(lua, std::get<Index>(vals).at(0));
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
        if constexpr(std::is_pointer_v<T> && !std::is_same_v<const char*, T>)
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
#define STR(X) #X
#define CONCAT(x, y) x##y
#define QuickRegisterImgui(X) Register(lua, #X, ImGui::X)
// Macro to register an overloaded function with the same name as the ImGui function
#define QuickRegisterImguiOverload(X, type) Register(lua, #X, static_cast<type>(ImGui::X))
// Macro to register an overloaded function with a different name then the ImGui function by
// appending a suffix to it
#define QuickRegisterImguiUOverload(X, suff, type) \
    Register(lua, #X #suff, static_cast<type>(ImGui::X))

        QuickRegisterImgui(Begin);
        QuickRegisterImgui(End);
        QuickRegisterImguiOverload(
            BeginChild,
            bool (*)(const char*, const ImVec2&, bool, ImGuiWindowFlags));
        QuickRegisterImguiUOverload(
            BeginChild,
            ID,
            bool (*)(ImGuiID, const ImVec2&, bool, ImGuiWindowFlags));
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
        QuickRegisterImguiOverload(SetWindowPos, void (*)(const char*, const ImVec2&, ImGuiCond));
        QuickRegisterImguiOverload(SetWindowSize, void (*)(const char*, const ImVec2&, ImGuiCond));
        QuickRegisterImguiOverload(SetWindowCollapsed, void (*)(const char*, bool, ImGuiCond));
        QuickRegisterImguiOverload(SetWindowFocus, void (*)(const char*));

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
        QuickRegisterImguiUOverload(PushStyleColor, Packed, void (*)(ImGuiCol, ImU32));
        QuickRegisterImguiOverload(PushStyleColor, void (*)(ImGuiCol, const ImVec4&));
        QuickRegisterImgui(PopStyleColor);
        QuickRegisterImguiOverload(PushStyleVar, void (*)(ImGuiStyleVar, float));
        QuickRegisterImguiUOverload(PushStyleVar, 2, void (*)(ImGuiStyleVar, const ImVec2& val));
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

        QuickRegisterImguiOverload(PushID, void (*)(const char*));
        // Rest of PushID seem c-specific
        QuickRegisterImgui(PopID);
        QuickRegisterImguiOverload(GetID, ImGuiID(*)(const char*));
        // Rest of GetID seem c-specific

        using VStr = Variadic<const char*>;

        // https://stackoverflow.com/questions/18889028/a-positive-lambda-what-sorcery-is-this?noredirect=1&lq=1
        // Below is the "correct" answer but the link above captures the feeling
        // of seeing +lambda for the first time
        // https://stackoverflow.com/questions/17822131
        Register(
            lua,
            "Text",
            +[](VStr var) { return ImGui::Text("%s", var.strcpy().data()); });
        Register(
            lua,
            "TextColored",
            +[](const ImVec4& col, VStr var) {
                return ImGui::TextColored(col, "%s", var.strcpy().data());
            });
        Register(
            lua,
            "TextDisabled",
            +[](VStr var) { return ImGui::TextDisabled("%s", var.strcpy().data()); });
        Register(
            lua,
            "TextWrapped",
            +[](VStr var) { return ImGui::TextWrapped("%s", var.strcpy().data()); });
        Register(
            lua,
            "LabelText",
            +[](const char* label, VStr var) {
                return ImGui::LabelText(label, "%s", var.strcpy().data());
            });
        Register(
            lua,
            "BulletText",
            +[](VStr var) { return ImGui::BulletText("%s", var.strcpy().data()); });

        QuickRegisterImgui(Button);
        QuickRegisterImgui(SmallButton);
        QuickRegisterImgui(InvisibleButton);
        QuickRegisterImgui(ArrowButton);
        // Image
        // ImageButton
        QuickRegisterImgui(Checkbox);
        // CheckboxFlags left out
        QuickRegisterImguiOverload(RadioButton, bool (*)(const char*, bool));
        QuickRegisterImguiUOverload(RadioButton, Mult, bool (*)(const char*, int*, int));
        QuickRegisterImgui(ProgressBar);
        QuickRegisterImgui(Bullet);

        QuickRegisterImgui(BeginCombo);
        QuickRegisterImgui(EndCombo);
        // Old Combo API left out on purpose

        QuickRegisterImgui(DragFloat);
        QuickRegisterImgui(DragFloat2);
        QuickRegisterImgui(DragFloat3);
        QuickRegisterImgui(DragFloat4);
        QuickRegisterImgui(DragFloatRange2);
        QuickRegisterImgui(DragInt);
        QuickRegisterImgui(DragInt2);
        QuickRegisterImgui(DragInt3);
        QuickRegisterImgui(DragInt4);
        QuickRegisterImgui(DragIntRange2);
        // DragScalar lef out on purpose

        QuickRegisterImgui(SliderFloat);
        QuickRegisterImgui(SliderFloat2);
        QuickRegisterImgui(SliderFloat3);
        QuickRegisterImgui(SliderFloat4);
        QuickRegisterImgui(SliderAngle);
        QuickRegisterImgui(SliderInt);
        QuickRegisterImgui(SliderInt2);
        QuickRegisterImgui(SliderInt3);
        QuickRegisterImgui(SliderInt4);
        QuickRegisterImgui(VSliderFloat);
        QuickRegisterImgui(VSliderInt);

        // InputText
        QuickRegisterImgui(InputFloat);
        QuickRegisterImgui(InputFloat2);
        QuickRegisterImgui(InputFloat3);
        QuickRegisterImgui(InputFloat4);
        QuickRegisterImgui(InputInt);
        QuickRegisterImgui(InputInt2);
        QuickRegisterImgui(InputInt3);
        QuickRegisterImgui(InputInt4);
        // InputDouble left out on purpose

        QuickRegisterImgui(ColorEdit3);
        QuickRegisterImgui(ColorEdit4);
        QuickRegisterImgui(ColorPicker3);
        QuickRegisterImgui(ColorPicker4);
        QuickRegisterImgui(ColorButton);
        QuickRegisterImgui(SetColorEditOptions);

        QuickRegisterImguiOverload(TreeNode, bool (*)(const char*));
        Register(
            lua,
            "TreeNodeID",
            +[](const char* id, VStr var) {
                return ImGui::TreeNode(id, "%s", var.strcpy().data());
            });
        Register(
            lua,
            "TreeNodeEx",
            static_cast<bool (*)(const char*, ImGuiTreeNodeFlags)>(ImGui::TreeNodeEx));
        Register(
            lua,
            "TreeNodeExID",
            +[](const char* id, ImGuiTreeNodeFlags flags, VStr var) {
                return ImGui::TreeNodeEx(id, flags, "%s", var.strcpy().data());
            });
        QuickRegisterImguiOverload(TreePush, void (*)(const char*));
        QuickRegisterImgui(TreePop);
        QuickRegisterImgui(GetTreeNodeToLabelSpacing);
        QuickRegisterImguiOverload(CollapsingHeader, bool (*)(const char*, ImGuiTreeNodeFlags));
        QuickRegisterImguiUOverload(
            CollapsingHeader,
            Toggle,
            bool (*)(const char*, bool*, ImGuiTreeNodeFlags));
        QuickRegisterImgui(SetNextItemOpen);

        QuickRegisterImguiOverload(
            Selectable,
            bool (*)(const char*, bool*, ImGuiSelectableFlags, const ImVec2&));

        QuickRegisterImgui(BeginListBox);
        QuickRegisterImgui(EndListBox);
        // ListBox left out on purpose

        // Plot functions left out on purpose, maybe they can be added at some point
        // Value left out on purpose

        QuickRegisterImgui(BeginMenuBar);
        QuickRegisterImgui(EndMenuBar);
        QuickRegisterImgui(BeginMainMenuBar);
        QuickRegisterImgui(EndMainMenuBar);
        QuickRegisterImgui(BeginMenu);
        QuickRegisterImgui(EndMenu);
        QuickRegisterImguiOverload(MenuItem, bool (*)(const char*, const char*, bool, bool));
        QuickRegisterImguiUOverload(
            MenuItem,
            Toggle,
            bool (*)(const char*, const char*, bool, bool));

        QuickRegisterImgui(BeginTooltip);
        QuickRegisterImgui(EndTooltip);

        QuickRegisterImgui(BeginPopup);
        QuickRegisterImgui(BeginPopupModal);
        QuickRegisterImgui(EndPopup);
        QuickRegisterImguiOverload(OpenPopup, void (*)(const char*, ImGuiPopupFlags));
        QuickRegisterImguiUOverload(OpenPopup, ID, void (*)(ImGuiID, ImGuiPopupFlags));
        QuickRegisterImgui(OpenPopupOnItemClick);
        QuickRegisterImgui(CloseCurrentPopup);
        QuickRegisterImgui(BeginPopupContextItem);
        QuickRegisterImgui(BeginPopupContextWindow);
        QuickRegisterImgui(BeginPopupContextVoid);
        QuickRegisterImgui(IsPopupOpen);

        QuickRegisterImgui(BeginTable);
        QuickRegisterImgui(EndTable);
        QuickRegisterImgui(TableNextRow);
        QuickRegisterImgui(TableNextColumn);
        QuickRegisterImgui(TableSetColumnIndex);
        QuickRegisterImgui(TableSetupColumn);
        QuickRegisterImgui(TableSetupScrollFreeze);
        QuickRegisterImgui(TableHeadersRow);
        QuickRegisterImgui(TableHeader);
        // TableGetSortSpecs
        QuickRegisterImgui(TableGetColumnCount);
        QuickRegisterImgui(TableGetColumnIndex);
        QuickRegisterImgui(TableGetRowIndex);
        QuickRegisterImgui(TableGetColumnName);
        QuickRegisterImgui(TableGetColumnFlags);
        QuickRegisterImgui(TableSetColumnEnabled);
        QuickRegisterImgui(TableSetBgColor);

        // Columns left out on purpose
    }
}