#pragma once

#include <entt/entity/fwd.hpp>
#include <numbers>
#include <numeric>
#include <vector>

#include <entity_reflection/reflection_entity.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register.hpp>
#include <lua_impl/lua_register_types.hpp>
#include <lua_impl/lua_validator.hpp>

#include <entity/area_tracker.hpp>
#define RComponent AreaTracker
EntityReflectionStruct(RComponent)
{
    ; // This semicolon needs to be here or else clang-format breaks. The benefits of the macro
      // outweigh the weirdness

    static void Create(entt::registry & registry, entt::entity entity)
    {
        registry.emplace<Component::RComponent>(entity, 0.0f, 0.0f, 0.0f);
    }

    static LuaValidator::LuaValidator GetLuaValidator(lua_State * lua)
    {
        return LuaValidator::LuaValidator(lua).FieldIs<Vector3>("offset").FieldIs<Vector3>("size");
    }

    static void CreateFromLuaInternal(
        lua_State * lua,
        entt::registry & registry,
        entt::entity entity)
    {
        lua_getfield(lua, -1, "offset");
        lua_getfield(lua, -2, "size");

        // Use a vector just in case, but I don't think there'll be this
        // many entities
        std::vector<entt::entity> vec;
        vec.reserve(16);

        auto stackTop = lua_gettop(lua);
        registry.emplace<Component::RComponent>(
            entity,
            Component::RComponent{
                .offset = LuaRegister::LuaGetFunc<Vector3>(lua, stackTop - 1),
                .size = LuaRegister::LuaGetFunc<Vector3>(lua, stackTop),
                .entitiesInside = vec,
            });

        lua_pop(lua, 2);
    }

    static void PushToLuaInternal(lua_State * lua, const Component::RComponent& component)
    {
        lua_pushstring(lua, "offset");
        LuaRegister::LuaSetFunc<Vector3>(lua, component.offset);
        lua_settable(lua, -3);
        lua_pushstring(lua, "size");
        LuaRegister::LuaSetFunc<Vector3>(lua, component.size);
        lua_settable(lua, -3);
    }

    static void View(Component::RComponent & component)
    {
        ImGui::Text(
            "Offset: %f, %f, %f",
            component.offset.x,
            component.offset.y,
            component.offset.z);

        ImGui::Text("Size: %f, %f, %f", component.size.x, component.size.y, component.size.z);
    }

    static void Modify(
        entt::registry & registry,
        entt::entity entity,
        Component::RComponent & component)
    {
        ImGui::DragFloat3("Offset", &component.offset.x);
        ImGui::DragFloat3("Size", &component.size.x);
    }

    static void Duplicate(
        entt::registry & registry,
        const Component::RComponent& component,
        entt::entity target)
    {
        registry.emplace<Component::RComponent>(
            target,
            Component::RComponent{.offset = component.offset, .size = component.size});
    }
};
EntityReflectionStructTail(RComponent)
#undef RComponent