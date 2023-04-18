#include "world.hpp"
#include "assets.hpp"
#include "entity/render.hpp"
#include "entity/transform.hpp"
#include "entity/velocity.hpp"
#include <algorithm>
#include <array>
#include <external/raylib.hpp>
#include <imgui.h>

#include <imgui/imgui_entity/entity_reflection.hpp>
#include <imgui/imgui_entity/imgui_render.hpp>
#include <imgui/imgui_entity/imgui_transform.hpp>
#include <imgui/imgui_entity/imgui_velocity.hpp>

struct WorldData
{
    entt::registry* registry;
} world;

// Should contain all components so they can be enumerated
namespace Component
{

    enum class Component : int
    {
        Render = 0,
        Transform,
        Velocity,
        LAST
    };
}
// std::array will at least give a compile error if the size of `Component` is modified
std::array<const char*, (int)Component::Component::LAST> ComponentString = {
    "Render",
    "Transform",
    "Velocity",
};

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
            world.registry->view<Component::Transform, Component::Render>().each())
        {
            DrawModel(render.model, transform.position, 1.0f, WHITE);
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
                bool hasComponents = true;
                EntityReflection::ModifyEntityOrElse(registry, entity, [&]() {
                    ImGui::Text("No components in this entity");
                    hasComponents = false;
                });

                bool anyMissing = EntityReflection::IsMissing<ImguiRender>(registry, entity)
                                  || EntityReflection::IsMissing<ImguiTransform>(registry, entity)
                                  || EntityReflection::IsMissing<ImguiVelocity>(registry, entity);

                if(anyMissing)
                {
                    if(ImGui::BeginCombo("##addcomponent", "Add component"))
                    {
                        EntityReflection::IfMissing<ImguiRender>(registry, entity, [&]() {
                            if(ImGui::Selectable("Render", selectedComponent == 0))
                            {
                                registry.emplace<Component::Render>(
                                    entity,
                                    GetAssetName(Asset::Insurgent),
                                    GetLoadedAsset(Asset::Insurgent));
                            }
                        });
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
            }
            ImGui::PopID();
        };
        world.registry->each(printEntity);
        ImGui::End();
    }
}