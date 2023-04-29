#pragma once

#include <entity_reflection/reflection_entity.hpp>
#include <entt/entt.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register.hpp>
#include <lua_impl/lua_register_types.hpp>

#include <entity/camera.hpp>
#define RComponent Camera
EntityReflectionStruct(RComponent)
{
    ; // This semicolon needs to be here or else clang-format breaks. The benefits of the macro
      // outweigh the weirdness

    static void Create(entt::registry & registry, entt::entity entity) {}

    static LuaValidator::LuaValidator GetLuaValidator(lua_State * lua)
    {
        return LuaValidator::LuaValidator(lua)
            .FieldIs<Vector3>("target")
            .FieldIs<Vector3>("up")
            .FieldIs<float>("fovy")
            .FieldIs<int>("projection");
    }

    static bool CreateFromLuaInternal(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity)
    {
        Component::RComponent component{};

        // TODO: Just allow negative indices in LuaGetFunc to skip this gettop + 1 nonsense
        auto table = lua_gettop(lua);

        lua_getfield(lua, table, "target");
        component.target = LuaRegister::LuaGetFunc<Vector3>(lua, table + 1);
        lua_getfield(lua, table, "up");
        component.up = LuaRegister::LuaGetFunc<Vector3>(lua, table + 2);
        lua_getfield(lua, table, "fovy");
        component.fovy = lua_tonumber(lua, -1);
        lua_getfield(lua, table, "projection");
        component.projection = lua_tointeger(lua, -1);

        lua_pop(lua, 4);

        registry.emplace<Component::RComponent>(entity, component);
        return true;
    }

    static void PushToLuaInternal(lua_State * lua, const Component::RComponent& component)
    {
        lua_pushstring(lua, "target");
        LuaRegister::LuaSetFunc<Vector3>(lua, component.target);
        lua_settable(lua, -3);
        lua_pushstring(lua, "up");
        LuaRegister::LuaSetFunc<Vector3>(lua, component.up);
        lua_settable(lua, -3);
        lua_pushstring(lua, "fovy");
        lua_pushnumber(lua, component.fovy);
        lua_settable(lua, -3);
        lua_pushstring(lua, "projection");
        lua_pushinteger(lua, component.projection);
        lua_settable(lua, -3);
    }

    static void View(Component::RComponent & component)
    {
        ImGui::Text(
            "Target: %f, %f, %f",
            component.target.x,
            component.target.y,
            component.target.z);
        ImGui::Text("Up: %f, %f, %f", component.up.x, component.up.y, component.up.z);
        ImGui::Text("FoV Vertical: %f", component.fovy);
        if(component.projection == CAMERA_PERSPECTIVE)
            ImGui::Text("Projection: Perspective");
        else
            ImGui::Text("Projection: Orthographic");
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component)
    {
        ImGui::InputFloat3("Target", &component.target.x);
        ImGui::InputFloat3("Up", &component.up.x);
        ImGui::DragFloat("FoV Vertical", &component.fovy, 1.0f, 20.0f, 150.0f);
        if(ImGui::RadioButton("Perspective", component.projection == CAMERA_PERSPECTIVE))
            component.projection = CAMERA_PERSPECTIVE;
        ImGui::SameLine();
        if(ImGui::RadioButton("Orthographic", component.projection == CAMERA_ORTHOGRAPHIC))
            component.projection = CAMERA_ORTHOGRAPHIC;
    }

    static void Duplicate(
        entt::registry & registry,
        const Component::RComponent& component,
        entt::entity target)
    {
        // Not yet
        // registry.emplace<Component::Camera>(target, component.x, component.y, component.z);
    }
};
EntityReflectionStructTail(RComponent)
#undef RComponent