#include "lua_imgui_impl.hpp"

#include "lua_register.hpp"
#include <imgui.h>

#define LuaImguiQuickRegister(X) LuaRegister::Register(lua, #X, ImGui::X)
// Macro to register an overloaded function with the same name as the ImGui function
#define LuaImguiQuickRegisterOverload(X, type) \
    LuaRegister::Register(lua, #X, static_cast<type>(ImGui::X))
// Macro to register an overloaded function with a different name then the ImGui function by
// appending a suffix to it
#define LuaImguiQuickRegisterUOverload(X, suff, type) \
    LuaRegister::Register(lua, #X #suff, static_cast<type>(ImGui::X))

namespace LuaRegister
{
    template<>
    inline auto GetDefault<ImVec2> = ImVec2(0.0f, 0.0f);
    template<>
    inline auto GetDefault<ImVec4> = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    template<>
    constexpr auto LuaSetFunc<ImVec2> = [](lua_State* lua, ImVec2 v) {
        lua_createtable(lua, 0, 2);
        lua_pushnumber(lua, v.x);
        lua_setfield(lua, -2, "x");
        lua_pushnumber(lua, v.y);
        lua_setfield(lua, -2, "y");
    };

    template<>
    constexpr auto LuaSetFunc<ImVec4> = [](lua_State* lua, ImVec4 v) {
        lua_createtable(lua, 0, 2);
        lua_pushnumber(lua, v.x);
        lua_setfield(lua, -2, "x");
        lua_pushnumber(lua, v.y);
        lua_setfield(lua, -2, "y");
        lua_pushnumber(lua, v.z);
        lua_setfield(lua, -2, "z");
        lua_pushnumber(lua, v.w);
        lua_setfield(lua, -2, "w");
    };

    template<>
    constexpr auto LuaGetFunc<ImVec2> = [](lua_State* lua, int& i) {
        if(i > lua_gettop(lua))
            return ImVec2{0.0f, 0.0f};

        lua_getfield(lua, i, "x");
        float x = lua_tonumber(lua, -1);
        lua_getfield(lua, i, "y");
        float y = lua_tonumber(lua, -1);
        lua_pop(lua, 2);
        i++;
        return ImVec2{x, y};
    };

    template<>
    constexpr auto LuaGetFunc<ImVec4> = [](lua_State* lua, int& i) {
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
    };
}

namespace LuaImGui
{
    void Register(lua_State* lua)
    {
        using namespace LuaRegister;

        LuaImguiQuickRegister(Begin);
        LuaImguiQuickRegister(End);
        LuaImguiQuickRegisterOverload(
            BeginChild,
            bool (*)(const char*, const ImVec2&, bool, ImGuiWindowFlags));
        LuaImguiQuickRegisterUOverload(
            BeginChild,
            ID,
            bool (*)(ImGuiID, const ImVec2&, bool, ImGuiWindowFlags));
        LuaImguiQuickRegister(EndChild);

        LuaImguiQuickRegister(IsWindowAppearing);
        LuaImguiQuickRegister(IsWindowCollapsed);
        LuaImguiQuickRegister(IsWindowFocused);
        LuaImguiQuickRegister(IsWindowHovered);
        // GetWindowDrawList
        LuaImguiQuickRegister(GetWindowDpiScale);
        LuaImguiQuickRegister(GetWindowPos);
        LuaImguiQuickRegister(GetWindowSize);
        LuaImguiQuickRegister(GetWindowWidth);
        LuaImguiQuickRegister(GetWindowHeight);
        // GetWindowViewport

        LuaImguiQuickRegister(SetNextWindowPos);
        LuaImguiQuickRegister(SetNextWindowSize);
        // SetNextWindowSizeConstraints
        LuaImguiQuickRegister(SetNextWindowContentSize);
        LuaImguiQuickRegister(SetNextWindowCollapsed);
        LuaImguiQuickRegister(SetNextWindowFocus);
        LuaImguiQuickRegister(SetNextWindowBgAlpha);
        // SetNextWindowViewport
        // SetWindowPos
        // SetWindowSize
        // SetWindowCollapsed
        // SetWindowFocus
        // SetWindowFontScale
        LuaImguiQuickRegisterOverload(
            SetWindowPos,
            void (*)(const char*, const ImVec2&, ImGuiCond));
        LuaImguiQuickRegisterOverload(
            SetWindowSize,
            void (*)(const char*, const ImVec2&, ImGuiCond));
        LuaImguiQuickRegisterOverload(SetWindowCollapsed, void (*)(const char*, bool, ImGuiCond));
        LuaImguiQuickRegisterOverload(SetWindowFocus, void (*)(const char*));

        LuaImguiQuickRegister(GetContentRegionAvail);
        LuaImguiQuickRegister(GetContentRegionMax);
        LuaImguiQuickRegister(GetWindowContentRegionMin);
        LuaImguiQuickRegister(GetWindowContentRegionMax);

        LuaImguiQuickRegister(GetScrollX);
        LuaImguiQuickRegister(GetScrollY);
        LuaImguiQuickRegister(SetScrollX);
        LuaImguiQuickRegister(SetScrollY);
        LuaImguiQuickRegister(GetScrollMaxX);
        LuaImguiQuickRegister(GetScrollMaxY);
        LuaImguiQuickRegister(SetScrollHereX);
        LuaImguiQuickRegister(SetScrollHereY);
        LuaImguiQuickRegister(SetScrollFromPosX);
        LuaImguiQuickRegister(SetScrollFromPosY);

        // PushFont(ImFont * font);
        // PopFont();
        LuaImguiQuickRegisterUOverload(PushStyleColor, Packed, void (*)(ImGuiCol, ImU32));
        LuaImguiQuickRegisterOverload(PushStyleColor, void (*)(ImGuiCol, const ImVec4&));
        LuaImguiQuickRegister(PopStyleColor);
        LuaImguiQuickRegisterOverload(PushStyleVar, void (*)(ImGuiStyleVar, float));
        LuaImguiQuickRegisterUOverload(PushStyleVar, 2, void (*)(ImGuiStyleVar, const ImVec2& val));
        LuaImguiQuickRegister(PopStyleVar);
        LuaImguiQuickRegister(PushAllowKeyboardFocus);
        LuaImguiQuickRegister(PopAllowKeyboardFocus);
        LuaImguiQuickRegister(PushButtonRepeat);
        LuaImguiQuickRegister(PopButtonRepeat);

        LuaImguiQuickRegister(PushItemWidth);
        LuaImguiQuickRegister(PopItemWidth);
        LuaImguiQuickRegister(SetNextItemWidth);
        LuaImguiQuickRegister(CalcItemWidth);
        LuaImguiQuickRegister(PushTextWrapPos);
        LuaImguiQuickRegister(PopTextWrapPos);

        LuaImguiQuickRegisterOverload(PushID, void (*)(const char*));
        // Rest of PushID seem c-specific
        LuaImguiQuickRegister(PopID);
        LuaImguiQuickRegisterOverload(GetID, ImGuiID(*)(const char*));
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

        LuaImguiQuickRegister(Button);
        LuaImguiQuickRegister(SmallButton);
        LuaImguiQuickRegister(InvisibleButton);
        LuaImguiQuickRegister(ArrowButton);
        // Image
        // ImageButton
        LuaImguiQuickRegister(Checkbox);
        // CheckboxFlags left out
        LuaImguiQuickRegisterOverload(RadioButton, bool (*)(const char*, bool));
        LuaImguiQuickRegisterUOverload(RadioButton, Mult, bool (*)(const char*, int*, int));
        LuaImguiQuickRegister(ProgressBar);
        LuaImguiQuickRegister(Bullet);

        LuaImguiQuickRegister(BeginCombo);
        LuaImguiQuickRegister(EndCombo);
        // Old Combo API left out on purpose

        LuaImguiQuickRegister(DragFloat);
        LuaImguiQuickRegister(DragFloat2);
        LuaImguiQuickRegister(DragFloat3);
        LuaImguiQuickRegister(DragFloat4);
        LuaImguiQuickRegister(DragFloatRange2);
        LuaImguiQuickRegister(DragInt);
        LuaImguiQuickRegister(DragInt2);
        LuaImguiQuickRegister(DragInt3);
        LuaImguiQuickRegister(DragInt4);
        LuaImguiQuickRegister(DragIntRange2);
        // DragScalar lef out on purpose

        LuaImguiQuickRegister(SliderFloat);
        LuaImguiQuickRegister(SliderFloat2);
        LuaImguiQuickRegister(SliderFloat3);
        LuaImguiQuickRegister(SliderFloat4);
        LuaImguiQuickRegister(SliderAngle);
        LuaImguiQuickRegister(SliderInt);
        LuaImguiQuickRegister(SliderInt2);
        LuaImguiQuickRegister(SliderInt3);
        LuaImguiQuickRegister(SliderInt4);
        LuaImguiQuickRegister(VSliderFloat);
        LuaImguiQuickRegister(VSliderInt);

        // InputText
        LuaImguiQuickRegister(InputFloat);
        LuaImguiQuickRegister(InputFloat2);
        LuaImguiQuickRegister(InputFloat3);
        LuaImguiQuickRegister(InputFloat4);
        LuaImguiQuickRegister(InputInt);
        LuaImguiQuickRegister(InputInt2);
        LuaImguiQuickRegister(InputInt3);
        LuaImguiQuickRegister(InputInt4);
        // InputDouble left out on purpose

        LuaImguiQuickRegister(ColorEdit3);
        LuaImguiQuickRegister(ColorEdit4);
        LuaImguiQuickRegister(ColorPicker3);
        LuaImguiQuickRegister(ColorPicker4);
        LuaImguiQuickRegister(ColorButton);
        LuaImguiQuickRegister(SetColorEditOptions);

        LuaImguiQuickRegisterOverload(TreeNode, bool (*)(const char*));
        LuaRegister::Register(
            lua,
            "TreeNodeID",
            +[](const char* id, VStr var) {
                return ImGui::TreeNode(id, "%s", var.strcpy().data());
            });
        LuaRegister::Register(
            lua,
            "TreeNodeEx",
            static_cast<bool (*)(const char*, ImGuiTreeNodeFlags)>(ImGui::TreeNodeEx));
        LuaRegister::Register(
            lua,
            "TreeNodeExID",
            +[](const char* id, ImGuiTreeNodeFlags flags, VStr var) {
                return ImGui::TreeNodeEx(id, flags, "%s", var.strcpy().data());
            });
        LuaImguiQuickRegisterOverload(TreePush, void (*)(const char*));
        LuaImguiQuickRegister(TreePop);
        LuaImguiQuickRegister(GetTreeNodeToLabelSpacing);
        LuaImguiQuickRegisterOverload(CollapsingHeader, bool (*)(const char*, ImGuiTreeNodeFlags));
        LuaImguiQuickRegisterUOverload(
            CollapsingHeader,
            Toggle,
            bool (*)(const char*, bool*, ImGuiTreeNodeFlags));
        LuaImguiQuickRegister(SetNextItemOpen);

        LuaImguiQuickRegisterOverload(
            Selectable,
            bool (*)(const char*, bool*, ImGuiSelectableFlags, const ImVec2&));

        LuaImguiQuickRegister(BeginListBox);
        LuaImguiQuickRegister(EndListBox);
        // ListBox left out on purpose

        // Plot functions left out on purpose, maybe they can be added at some point
        // Value left out on purpose

        LuaImguiQuickRegister(BeginMenuBar);
        LuaImguiQuickRegister(EndMenuBar);
        LuaImguiQuickRegister(BeginMainMenuBar);
        LuaImguiQuickRegister(EndMainMenuBar);
        LuaImguiQuickRegister(BeginMenu);
        LuaImguiQuickRegister(EndMenu);
        LuaImguiQuickRegisterOverload(MenuItem, bool (*)(const char*, const char*, bool, bool));
        LuaImguiQuickRegisterUOverload(
            MenuItem,
            Toggle,
            bool (*)(const char*, const char*, bool, bool));

        LuaImguiQuickRegister(BeginTooltip);
        LuaImguiQuickRegister(EndTooltip);

        LuaImguiQuickRegister(BeginPopup);
        LuaImguiQuickRegister(BeginPopupModal);
        LuaImguiQuickRegister(EndPopup);
        LuaImguiQuickRegisterOverload(OpenPopup, void (*)(const char*, ImGuiPopupFlags));
        LuaImguiQuickRegisterUOverload(OpenPopup, ID, void (*)(ImGuiID, ImGuiPopupFlags));
        LuaImguiQuickRegister(OpenPopupOnItemClick);
        LuaImguiQuickRegister(CloseCurrentPopup);
        LuaImguiQuickRegister(BeginPopupContextItem);
        LuaImguiQuickRegister(BeginPopupContextWindow);
        LuaImguiQuickRegister(BeginPopupContextVoid);
        LuaImguiQuickRegister(IsPopupOpen);

        LuaImguiQuickRegister(BeginTable);
        LuaImguiQuickRegister(EndTable);
        LuaImguiQuickRegister(TableNextRow);
        LuaImguiQuickRegister(TableNextColumn);
        LuaImguiQuickRegister(TableSetColumnIndex);
        LuaImguiQuickRegister(TableSetupColumn);
        LuaImguiQuickRegister(TableSetupScrollFreeze);
        LuaImguiQuickRegister(TableHeadersRow);
        LuaImguiQuickRegister(TableHeader);
        // TableGetSortSpecs
        LuaImguiQuickRegister(TableGetColumnCount);
        LuaImguiQuickRegister(TableGetColumnIndex);
        LuaImguiQuickRegister(TableGetRowIndex);
        LuaImguiQuickRegister(TableGetColumnName);
        LuaImguiQuickRegister(TableGetColumnFlags);
        LuaImguiQuickRegister(TableSetColumnEnabled);
        LuaImguiQuickRegister(TableSetBgColor);

        // Columns left out on purpose

        LuaImguiQuickRegister(BeginTabBar);
        LuaImguiQuickRegister(EndTabBar);
        LuaImguiQuickRegister(BeginTabItem);
        LuaImguiQuickRegister(EndTabItem);
        LuaImguiQuickRegister(TabItemButton);
        LuaImguiQuickRegister(SetTabItemClosed);

        LuaImguiQuickRegister(BeginDisabled);
        LuaImguiQuickRegister(EndDisabled);

        LuaImguiQuickRegister(SetItemDefaultFocus);
        LuaImguiQuickRegister(SetKeyboardFocusHere);

        LuaImguiQuickRegister(IsItemHovered);
        LuaImguiQuickRegister(IsItemActive);
        LuaImguiQuickRegister(IsItemFocused);
        LuaImguiQuickRegister(IsItemClicked);
        LuaImguiQuickRegister(IsItemVisible);
        LuaImguiQuickRegister(IsItemEdited);
        LuaImguiQuickRegister(IsItemActivated);
        LuaImguiQuickRegister(IsItemDeactivated);
        LuaImguiQuickRegister(IsItemDeactivatedAfterEdit);
        LuaImguiQuickRegister(IsItemToggledOpen);
        LuaImguiQuickRegister(IsAnyItemHovered);
        LuaImguiQuickRegister(IsAnyItemActive);
        LuaImguiQuickRegister(IsAnyItemFocused);
        LuaImguiQuickRegister(GetItemRectMin);
        LuaImguiQuickRegister(GetItemRectMax);
        LuaImguiQuickRegister(GetItemRectSize);
        LuaImguiQuickRegister(SetItemAllowOverlap);

        // Lots of missing stuff here that has to do with draw lists
        LuaImguiQuickRegister(GetTime);
        LuaImguiQuickRegister(GetFrameCount);
        LuaImguiQuickRegister(GetStyleColorName);
        LuaImguiQuickRegister(BeginChildFrame);
        LuaImguiQuickRegister(EndChildFrame);
        LuaImguiQuickRegister(CalcTextSize);
        LuaImguiQuickRegister(ColorConvertU32ToFloat4);
        LuaImguiQuickRegister(ColorConvertFloat4ToU32);
        LuaImguiQuickRegister(ColorConvertRGBtoHSV);
        LuaImguiQuickRegister(ColorConvertHSVtoRGB);

        LuaImguiQuickRegister(IsKeyDown);
        LuaImguiQuickRegister(IsKeyPressed);
        LuaImguiQuickRegister(IsKeyReleased);
        LuaImguiQuickRegister(GetKeyPressedAmount);
        LuaImguiQuickRegister(GetKeyName);
        LuaImguiQuickRegister(CaptureKeyboardFromApp);

        LuaImguiQuickRegister(IsMouseDown);
        LuaImguiQuickRegister(IsMouseClicked);
        LuaImguiQuickRegister(IsMouseReleased);
        LuaImguiQuickRegister(IsMouseDoubleClicked);
        LuaImguiQuickRegister(GetMouseClickedCount);
        LuaImguiQuickRegister(IsMouseHoveringRect);
        // LuaImguiQuickRegister(IsMousePosValid); // Takes a pointer that shouldn't actually be
        // returned, skip it for now until it is required
        // IsAnyMouseDown will be obsolete
        LuaImguiQuickRegister(GetMousePos);
        LuaImguiQuickRegister(GetMousePosOnOpeningCurrentPopup);
        LuaImguiQuickRegister(IsMouseDragging);
        LuaImguiQuickRegister(GetMouseDragDelta);
        LuaImguiQuickRegister(ResetMouseDragDelta);
        LuaImguiQuickRegister(GetMouseCursor);
        LuaImguiQuickRegister(SetMouseCursor);
        LuaImguiQuickRegister(CaptureMouseFromApp);

        LuaImguiQuickRegister(GetClipboardText);
        LuaImguiQuickRegister(SetClipboardText);

        LuaImguiQuickRegister(LoadIniSettingsFromDisk);
        LuaImguiQuickRegister(LoadIniSettingsFromMemory);
        LuaImguiQuickRegister(SaveIniSettingsToDisk);
        LuaImguiQuickRegister(SaveIniSettingsToMemory);

        // And that's everything
    }
}