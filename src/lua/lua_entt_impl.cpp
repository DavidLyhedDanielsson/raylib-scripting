#include "lua_entt_impl.hpp"

extern "C" {
#include <lua.h>
}
#include "lua_register.hpp"

#include <array>
#include <functional>
#include <type_traits>

#include "../assets.hpp"
#include "../entity/render.hpp"
#include "../entity/transform.hpp"
#include "../entity/velocity.hpp"

#define DeclareRegistry auto registry = (entt::registry*)lua_touserdata(lua, lua_upvalueindex(1))
#define LuaFunc(Name) extern "C" int Name(lua_State* lua)

namespace LuaEntt
{
    void RegisterTypes(lua_State* lua)
    {
        // Available models
        lua_newtable(lua);
        for(int i = 0; i < (int)Asset::Last; ++i)
        {
            lua_pushstring(lua, GetAssetName((Asset)i));
            lua_pushnumber(lua, i);
            lua_settable(lua, -3);
        }
        lua_setglobal(lua, "Asset");
    }

    lua_Integer CreateEntity(entt::registry* registry)
    {
        return static_cast<lua_Integer>(registry->create());
    }

    void AddRenderComponent(entt::registry* registry, lua_Integer entity, int assetId)
    {
        auto asset = (Asset)assetId;
        registry->emplace<Component::Render>(
            (entt::entity)entity,
            GetAssetName(asset),
            GetLoadedAsset(asset));
    }

    void AddTransformComponent(entt::registry* registry, lua_Integer entity)
    {
        registry->emplace<Component::Transform>(
            (entt::entity)entity,
            Vector3{0.0f, 0.0f, 0.0f},
            QuaternionIdentity());
    }

    void AddVelocityComponent(entt::registry* registry, lua_Integer entity)
    {
        registry->emplace<Component::Velocity>((entt::entity)entity, 0.0f, 0.0f, 0.0f);
    }

    void Register(lua_State* lua, entt::registry* registry)
    {
        RegisterTypes(lua);

#define QuickRegister(Func) LuaRegister::RegisterMember(lua, #Func, registry, Func);

        QuickRegister(CreateEntity);
        QuickRegister(AddRenderComponent);
        QuickRegister(AddTransformComponent);
        QuickRegister(AddVelocityComponent);
    }
}