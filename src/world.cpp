#include "world.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <entt/entity/utility.hpp>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>

#include <assets.hpp>
#include <component/behaviour.hpp>
#include <external/raylib.hpp>
#include <lua_impl/lua_register.hpp>
#include <lua_impl/lua_register_types.hpp>
#include <profiling.hpp>
#include <system/align_tiles.hpp>
#include <system/area_tracker.hpp>
#include <system/avoid_entities.hpp>
#include <system/avoid_obstacles.hpp>
#include <system/calculate_velocity.hpp>
#include <system/check_health.hpp>
#include <system/draw_renderables.hpp>
#include <system/max_range.hpp>
#include <system/move_entities.hpp>
#include <system/navigate.hpp>
#include <system/update_projectiles.hpp>

namespace World
{
    WorldState state;

    void Init()
    {
        // When creating a Behaviour component, the attached script needs to execute independently
        // of everything else, so store it in a lua table and call their function in World::Update
        state.registry->on_construct<Component::Behaviour>()
            .connect<[](entt::registry& registry, entt::entity entity) {
                Component::Behaviour behaviour = registry.get<Component::Behaviour>(entity);

                if(behaviour.script == "")
                    return;

                auto filePath = BehaviourFilePath(behaviour.script.c_str());
                {
                    std::ifstream in(filePath.data());
                    if(!in.is_open())
                    {
                        std::cerr << "Behaviour file " << behaviour.script << " doesn't exist"
                                  << std::endl;
                        return;
                    }
                }

                Profiling::ProfileCall("Load behaviour", [&]() {
                    auto res = luaL_dofile(state.lua, filePath.data());
                    if(res != LUA_OK)
                    {
                        std::cerr << "Couldn't load " << filePath.data() << " or error occurred"
                                  << std::endl;
                        std::cerr << lua_tostring(state.lua, -1) << std::endl;
                    }
                    else
                    {
                        if(!lua_isfunction(state.lua, -1))
                        {
                            std::cerr << "Behaviour file does not return a function";
                            return;
                        }

                        lua_rawgeti(state.lua, LUA_REGISTRYINDEX, state.behaviourTable);
                        lua_rotate(state.lua, -2, 1);
                        lua_setfield(state.lua, -2, filePath.data());
                        lua_pop(state.lua, 1);
                    }
                });
            }>();
        state.registry->on_destroy<Component::Behaviour>()
            .connect<[](entt::registry& registry, entt::entity entity) {
                Component::Behaviour behaviour = registry.get<Component::Behaviour>(entity);

                if(behaviour.script == "")
                    return;

                auto filePath = BehaviourFilePath(behaviour.script.c_str());
                lua_rawgeti(state.lua, LUA_REGISTRYINDEX, state.behaviourTable);
                lua_pushnil(state.lua);
                lua_setfield(state.lua, -2, filePath.data());
                lua_pop(state.lua, 1);
            }>();

        lua_createtable(state.lua, 0, 0);

        // I don't like this being here. It magically sets a global variables that is "owned" by
        // main. The ownership is unclear and I don't feel like coding up something more robust for
        // this small game
        state.behaviourTable = luaL_ref(state.lua, LUA_REGISTRYINDEX);
    }

    void Update()
    {
        // TODO :)
        // The obstacle avoidance wants a nice, consistent frame rate, so here it is! The game
        // always runs at 60 FPS no matter the FPS, raylib provides the target framerate of 60
        float time = 1.0f / 60.0f;
        auto lua = state.lua;

        // Run any global scripts, aka behaviour scripts
        lua_rawgeti(lua, LUA_REGISTRYINDEX, state.behaviourTable);
        auto table = lua_gettop(lua);

        lua_pushnil(lua);
        while(lua_next(lua, table) != 0)
        {
            if(lua_pcall(lua, 0, 0, 0) != LUA_OK)
            {
                std::cerr << "Error executing behaviour script: " << lua_tostring(lua, -1)
                          << std::endl;
            }
        }

        // These variables are tweakable from imgui for easy experimentation with navigation
        lua_getglobal(lua, "Navigation");
        lua_getfield(lua, -1, "ksi");
        const float ksi = (float)lua_tonumber(lua, -1);
        lua_pop(lua, 1);
        lua_getfield(lua, -1, "avoidanceLookAhead");
        const float avoidanceT = (float)lua_tonumber(lua, -1);
        lua_pop(lua, 1);
        lua_getfield(lua, -1, "obstacleLookAhead");
        const float obstacleT = (float)lua_tonumber(lua, -1);
        lua_pop(lua, 2);

        PROFILE_CALL(System::Navigate, *state.registry, state.navigation, ksi, time);
        PROFILE_CALL(System::AvoidEntities, *state.registry, ksi, avoidanceT, time);
        PROFILE_CALL(System::AvoidObstacles, *state.registry, state.navigation, obstacleT, time);
        PROFILE_CALL(System::CalculateVelocity, *state.registry, time);
        PROFILE_CALL(System::MoveEntities, *state.registry, time);
        PROFILE_CALL(System::AlignTiles, *state.registry);
        PROFILE_CALL(System::UpdateProjectiles, *state.registry);
        PROFILE_CALL(System::MaxRange, *state.registry);
        PROFILE_CALL(System::CheckHealth, *state.registry);
        PROFILE_CALL(System::UpdateAreaTrackers, *state.registry);
    }

    void Draw()
    {
        PROFILE_CALL(System::DrawRenderable, *state.registry);

        // Debugging visualisation
        // state.registry->view<Component::Transform, Component::AreaTracker>().each(
        //     [](entt::entity entity,
        //        Component::Transform transform,
        //        Component::AreaTracker tracker) {
        //         auto boundingBox = tracker.GetBoundingBox(transform);
        //         Vector3 center = Vector3Scale(Vector3Add(boundingBox.min, boundingBox.max),
        //         0.5f); DrawCubeV(
        //             center,
        //             Vector3Subtract(boundingBox.max, boundingBox.min),
        //             {255, 0, 0, 80});
        //     });

        // Some debugging visualisations that are controllable from imgui
        auto lua = state.lua;
        lua_getglobal(lua, "Navigation");
        lua_getfield(lua, -1, "drawTiles");
        state.drawNavigationTiles = lua_toboolean(lua, -1);
        lua_pop(lua, 2);
        if(state.drawNavigationTiles)
            state.navigation.DrawTiles();

        lua_getglobal(lua, "Navigation");
        if(lua_getfield(lua, -1, "drawField") == LUA_TNUMBER)
            state.drawNavigationField = (int)lua_tointeger(lua, -1);
        else
            state.drawNavigationField = std::nullopt;
        lua_pop(lua, 2);
        if(state.drawNavigationField)
            state.navigation.DrawField(state.drawNavigationField.value());
    }
}