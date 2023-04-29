#include "lua_asset_impl.hpp"

#include <assets.hpp>
#include <lua_impl/lua_register.hpp>

namespace LuaAsset
{
    void Register(lua_State* lua)
    {
        using namespace LuaRegister;

        lua_createtable(lua, loadedAssets.size(), 0);
        int i = 1;
        for(const auto& [path, model] : loadedAssets)
        {
            lua_pushinteger(lua, i);
            lua_pushstring(lua, path.c_str());
            lua_settable(lua, -3);

            ++i;
        }
        lua_setglobal(lua, "Assets");
    }
}