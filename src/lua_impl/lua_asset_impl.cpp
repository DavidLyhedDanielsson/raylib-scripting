#include "lua_asset_impl.hpp"

#include <assets.hpp>
#include <filesystem>
#include <fstream>
#include <lua_impl/lua_register.hpp>

namespace LuaAsset
{
    void Register(lua_State* lua)
    {
        lua_createtable(lua, 0, 0);

        lua_createtable(lua, loadedAssets.size(), 0);
        int i = 1;
        for(const auto& [path, model] : loadedAssets)
        {
            lua_pushstring(lua, path.c_str());
            lua_seti(lua, -2, i);

            ++i;
        }
        lua_setfield(lua, -2, "Meshes");

        LuaRegister::PushRegister(
            lua,
            "GetLevels",
            +[](lua_State* lua) {
                lua_createtable(lua, 0, 0);

                int levelNumber = 1;
                for(const auto& entry : std::filesystem::directory_iterator(
                        std::filesystem::path(DASSET_ROOT) / "levels"))
                {
                    if(!entry.is_regular_file())
                        continue;

                    if(entry.path().extension() != ".lua")
                        continue;

                    lua_pushinteger(lua, levelNumber++);
                    lua_pushstring(lua, entry.path().filename().string().c_str());
                    lua_settable(lua, -3);
                }

                return LuaRegister::Placeholder{};
            });

        LuaRegister::PushRegister(
            lua,
            "GetLevelFileContents",
            +[](lua_State* lua, const char* name) {
                auto file = std::filesystem::path(DASSET_ROOT) / "levels" / name;
                std::ifstream in(file, std::ios::ate);

                if(!in.is_open())
                {
                    size_t size = in.tellg();
                    std::vector<char> bytes(size + 1, '\0');
                    in.seekg(0);
                    in.read(bytes.data(), size);
                    lua_pushstring(lua, bytes.data());
                }
                else
                    lua_pushstring(lua, "");

                return LuaRegister::Placeholder{};
            });

        lua_setglobal(lua, "Assets");
    }
}