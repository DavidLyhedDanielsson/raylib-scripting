#include "world.hpp"
#include <raylib.h>
#include "entity/transform.hpp"
#include "entity/render.hpp"
#include "entity/velocity.hpp"
#include "assets.hpp"
#include <imgui.h>
#include <array>
#include <algorithm>

struct WorldData
{
    entt::registry *registry;
} world;

// Should contain all components so they can be enumerated
namespace Component
{

    enum class Component : int
    {
        Transform = 0,
        Velocity,
        LAST
    };
}
// std::array will at least give a compile error if the size of `Component` is modified
std::array<const char *, (int)Component::Component::LAST> ComponentString = {"Transform", "Velocity"};

namespace World
{
    void Init(entt::registry *registry)
    {
        // Insurgent comes from https://quaternius.com/. Thanks Quaternius!
        world.registry = registry;
        auto entity = world.registry->create();
        world.registry->emplace<Component::Transform>(entity, Vector3{0.0f, 0.0f, 0.0f}, QuaternionIdentity());
        world.registry->emplace<Component::Velocity>(entity, 0.0f, 0.0f, 0.0f);
        world.registry->emplace<Component::Render>(entity, GetAssetName(Asset::Insurgent), GetLoadedAsset(Asset::Insurgent));
    }

    void Update()
    {
        for (auto [entity, transform, velocity] : world.registry->view<Component::Transform, Component::Velocity>().each())
        {
            transform.position.x += velocity.x;
            transform.position.y += velocity.y;
            transform.position.z += velocity.z;
        }
    }

    void Draw()
    {
        for (auto [entity, transform, render] : world.registry->view<Component::Transform, Component::Render>().each())
        {
            DrawModel(render.model, transform.position, 1.0f, WHITE);
        }
    }

    void DrawImgui()
    {
        static int selectedComponent = 0;

        ImGui::Begin("Entities");
        auto printEntity = [](const entt::entity entity)
        {
            if (ImGui::TreeNode("Entity"))
            {

                ::std::array<void *, (int)Component::Component::LAST> entityComponents = {};
                // helper to reduce typing and potential copy-paste errors
#define SetComponent(T) entityComponents[(int)Component::T] = world.registry->try_get<T>(entity)

                SetComponent(Component::Transform);
                SetComponent(Component::Velocity);

                // Display a list of components that the entity does not already have
                if (std::find(entityComponents.begin(), entityComponents.end(), nullptr) != entityComponents.end())
                {
                    if (ImGui::BeginCombo("##addcomponent", "Add component"))
                    {
                        for (auto i = 0; i < entityComponents.size(); ++i)
                        {
                            if (!entityComponents[i] && ImGui::Selectable(ComponentString[i], i == selectedComponent))
                            {
                                switch (i)
                                {
                                case (int)Component::Component::Transform:
                                    world.registry->emplace<Component::Transform>(entity, 0.0f, 0.0f, 0.0f);
                                    break;
                                case (int)Component::Component::Velocity:
                                    world.registry->emplace<Component::Velocity>(entity, 0.0f, 0.0f, 0.0f);
                                    break;
                                default:
                                    break;
                                }
                            }
                        }
                        ImGui::EndCombo();
                    }
                }
                else
                {
                    ImGui::BeginDisabled();
                    ImGui::BeginCombo("##addentity", "No more components available");
                    ImGui::EndDisabled();
                }

                if (auto transform = (Component::Transform *)entityComponents[(int)Component::Component::Transform]; transform)
                {
                    ImGui::InputFloat3("Position", &(transform->position.x));
                }

                if (auto velocity = entityComponents[(int)Component::Component::Velocity]; velocity)
                {
                    ImGui::DragFloat3("Velocity", (float *)velocity, 0.01f, -25.0f, 25.0f);

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0xcc / 255.0f, 0x24 / 255.0f, 0x1d / 255.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0xd1 / 255.0f, 0x39 / 255.0f, 0x33 / 255.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0xb7 / 255.0f, 0x20 / 255.0f, 0x1a / 255.0f, 1.0f));
                    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 30.0f);
                    if (ImGui::Button("[X]"))
                    {
                        world.registry->remove<Component::Velocity>(entity);
                    }
                    ImGui::PopStyleColor(3);
                }

                ImGui::TreePop();
            }
        };
        world.registry->each(printEntity);
        ImGui::End();
    }
}