#include "world.hpp"
#include "assets.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <entity/camera.hpp>
#include <entity/render.hpp>
#include <entity/tile.hpp>
#include <entity/transform.hpp>
#include <entity/velocity.hpp>
#include <entity_reflection/entity_reflection.hpp>
#include <entity_reflection/reflection_camera.hpp>
#include <entity_reflection/reflection_render.hpp>
#include <entity_reflection/reflection_tile.hpp>
#include <entity_reflection/reflection_transform.hpp>
#include <entity_reflection/reflection_velocity.hpp>
#include <external/raylib.hpp>
#include <imgui.h>

#include <ImGuizmo.h>

struct WorldData
{
    entt::registry* registry;
} world;

namespace World
{
    void Init(entt::registry* registry)
    {
        world.registry = registry;
    }

    void Update()
    {
        for(auto [entity, transform, velocity] :
            world.registry->view<Component::Transform, Component::Velocity>().each())
        {
            transform.position.x += velocity.x;
            transform.position.y += velocity.y;
            transform.position.z += velocity.z;
        }

        for(auto [entity, transform] :
            world.registry->view<Component::Transform, Component::Tile>().each())
        {
            transform.position.x = std::roundf(transform.position.x);
            transform.position.y = std::roundf(transform.position.y);
            transform.position.z = std::roundf(transform.position.z);
        }
    }

    void Draw()
    {
        auto group = world.registry->group<Component::Render, Component::Transform>();

        for(auto entity : group)
        {
            auto [render, transform] = group.get<Component::Render, Component::Transform>(entity);
            DrawModel(render.model, transform.position, 1.0f, WHITE);
        }
    }

    void DrawImgui()
    {
        static uint32_t selectedComponent = 0;

        auto& registry = *world.registry;

        for(auto [entity, transform, render] :
            world.registry->view<Component::Transform, Component::Render>().each())
        {
            Camera3D camera;
            for(auto [_, transformC, cameraC] :
                world.registry->view<Component::Transform, Component::Camera>().each())
            {
                camera = cameraC.CreateRaylibCamera(transformC.position);
            }

            float zero[3] = {0.0f, 0.0f, 0.0f};
            float one[3] = {1.0f, 1.0f, 1.0f};
            float matrix[16] = {};

            ImGuizmo::RecomposeMatrixFromComponents(&transform.position.x, zero, one, matrix);

            const double RL_CULL_DISTANCE_NEAR =
                0.01; // Default projection matrix near cull distance
            const double RL_CULL_DISTANCE_FAR =
                1000.0; // Default projection matrix far cull distance

            auto viewMatrix = MatrixTranspose(GetCameraMatrix(camera));
            auto projMatrix = MatrixTranspose(MatrixPerspective(
                camera.fovy * DEG2RAD,
                ((double)GetScreenWidth() / (double)GetScreenHeight()),
                RL_CULL_DISTANCE_NEAR,
                RL_CULL_DISTANCE_FAR));

            auto identity = MatrixIdentity();

            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetRect(0.0f, 0.0f, GetScreenWidth(), GetScreenHeight());
            ImGuizmo::Enable(true);
            ImGuizmo::Manipulate(
                &viewMatrix.m0,
                &projMatrix.m0,
                ImGuizmo::OPERATION::TRANSLATE,
                ImGuizmo::MODE::WORLD,
                matrix);

            ImGuizmo::DecomposeMatrixToComponents(matrix, &transform.position.x, zero, one);
            break;
        }

        ImGui::Begin("Entities");
        auto printEntity = [&](const entt::entity entity) {
            assert((ENTT_ID_TYPE)entity <= std::numeric_limits<uint32_t>::max());
            ImGui::PushID((int32_t)entity);
            if(ImGui::TreeNode("Entity", "Entity %i", entity))
            {
                ImGui::PushStyleColor(
                    ImGuiCol_Button,
                    ImVec4(0xcc / 255.0f, 0x24 / 255.0f, 0x1d / 255.0f, 1.0f));
                ImGui::PushStyleColor(
                    ImGuiCol_ButtonHovered,
                    ImVec4(0xd1 / 255.0f, 0x39 / 255.0f, 0x33 / 255.0f, 1.0f));
                ImGui::PushStyleColor(
                    ImGuiCol_ButtonActive,
                    ImVec4(0xb7 / 255.0f, 0x20 / 255.0f, 0x1a / 255.0f, 1.0f));

                // Button should be here, but the entity is used below, so wait before deleting
                bool deleteEntity = ImGui::Button("DELETE ENTITY");
                ImGui::PopStyleColor(3);

                bool hasComponents = true;
                EntityReflection::ModifyEntityOrElse(registry, entity, [&]() {
                    ImGui::Text("No components in this entity");
                    hasComponents = false;
                });

                if(hasComponents)
                {
                    if(ImGui::Button("Duplicate entity"))
                        EntityReflection::DuplicateEntity(registry, entity);
                }

                bool anyMissing =
                    EntityReflection::IsMissing<RenderReflection>(registry, entity)
                    || EntityReflection::IsMissing<TransformReflection>(registry, entity)
                    || EntityReflection::IsMissing<VelocityReflection>(registry, entity)
                    || EntityReflection::IsMissing<TileReflection>(registry, entity);

                if(anyMissing)
                {
                    if(ImGui::BeginCombo("##addcomponent", "Add component"))
                    {
                        if(!loadedAssets.empty())
                        {
                            EntityReflection::IfMissing<RenderReflection>(registry, entity, [&]() {
                                if(ImGui::Selectable("Render", selectedComponent == 0))
                                {
                                    auto firstAsset = loadedAssets.begin();
                                    registry.emplace<Component::Render>(
                                        entity,
                                        firstAsset->first.c_str(),
                                        firstAsset->second);
                                }
                            });
                        }
                        else
                            ImGui::Text("No available models");
                        EntityReflection::IfMissing<TransformReflection>(registry, entity, [&]() {
                            if(ImGui::Selectable("Transform", selectedComponent == 1))
                            {
                                registry.emplace<Component::Transform>(entity, 0.0f, 0.0f, 0.0f);
                            }
                        });
                        EntityReflection::IfMissing<VelocityReflection>(registry, entity, [&]() {
                            if(ImGui::Selectable("Velocity", selectedComponent == 2))
                            {
                                registry.emplace<Component::Velocity>(entity, 0.0f, 0.0f, 0.0f);
                            }
                        });
                        EntityReflection::IfMissing<TileReflection>(registry, entity, [&]() {
                            if(ImGui::Selectable("Tile", selectedComponent == 3))
                            {
                                registry.emplace<Component::Tile>(entity);
                            }
                        });
                        ImGui::EndCombo();
                    }
                }
                else
                {
                    ImGui::BeginDisabled();
                    ImGui::BeginCombo("##addcomponent", "No more components available");
                    ImGui::EndDisabled();
                }

                ImGui::TreePop();

                if(deleteEntity)
                    registry.destroy(entity);
            }
            ImGui::PopID();
        };
        world.registry->each(printEntity);
        ImGui::End();
    }
}