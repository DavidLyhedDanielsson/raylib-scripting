#include "world.hpp"
#include "assets.hpp"
#include "entity/render.hpp"
#include "entity/transform.hpp"
#include "entity/velocity.hpp"
#include <algorithm>
#include <array>
#include <external/raylib.hpp>
#include <imgui.h>

#include <entity_reflection/entity_reflection.hpp>
#include <entity_reflection/reflection_render.hpp>
#include <entity_reflection/reflection_transform.hpp>
#include <entity_reflection/reflection_velocity.hpp>

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
    }

    void Draw()
    {
        for(auto [entity, transform, render] :
            world.registry
                ->view<Component::Transform, Component::Render>(entt::exclude<Component::Velocity>)
                .each())
        {
            BeginBlendMode(BLEND_ALPHA);
            DrawModel(render.model, transform.position, 1.0f, WHITE);
            EndBlendMode();
        }
    }

    void DrawImgui()
    {
        static uint32_t selectedComponent = 0;

        auto& registry = *world.registry;

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

                bool anyMissing = EntityReflection::IsMissing<ImguiRender>(registry, entity)
                                  || EntityReflection::IsMissing<ImguiTransform>(registry, entity)
                                  || EntityReflection::IsMissing<ImguiVelocity>(registry, entity);

                if(anyMissing)
                {
                    if(ImGui::BeginCombo("##addcomponent", "Add component"))
                    {
                        if(!loadedAssets.empty())
                        {
                            EntityReflection::IfMissing<ImguiRender>(registry, entity, [&]() {
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
                        EntityReflection::IfMissing<ImguiTransform>(registry, entity, [&]() {
                            if(ImGui::Selectable("Transform", selectedComponent == 1))
                            {
                                registry.emplace<Component::Transform>(entity, 0.0f, 0.0f, 0.0f);
                            }
                        });
                        EntityReflection::IfMissing<ImguiVelocity>(registry, entity, [&]() {
                            if(ImGui::Selectable("Velocity", selectedComponent == 2))
                            {
                                registry.emplace<Component::Velocity>(entity, 0.0f, 0.0f, 0.0f);
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