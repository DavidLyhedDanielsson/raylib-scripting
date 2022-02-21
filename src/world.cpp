#include "world.hpp"
#include <raylib.h>
#include <entt/entt.hpp>
#include "entity/position.hpp"
#include "entity/render.hpp"
#include "entity/velocity.hpp"
#include "assets.hpp"
#include <imgui.h>
#include <array>
#include <algorithm>

struct WorldData
{
    entt::registry registry;
} world;

// Should contain all components so they can be enumerated
enum class Component : int
{
    Position = 0,
    Velocity,
    LAST
};
// std::array will at least give a compile error if the size of `Component` is modified
std::array<const char *, (int)Component::LAST> ComponentString = {"Position", "Velocity"};

namespace World
{
    void Init()
    {
        // Insurgent comes from https://quaternius.com/. Thanks Quaternius!
        const auto insurgent = LoadModel(AssetPath("Insurgent/glTF/Insurgent.gltf").data());

        const auto entity = world.registry.create();
        world.registry.emplace<Position>(entity, 0.0f, 0.0f, 0.0f);
        world.registry.emplace<Velocity>(entity, 0.0f, 0.0f, 0.0f);
        world.registry.emplace<Render>(entity, insurgent);
    }

    void Update()
    {
        for (auto [entity, position, velocity] : world.registry.view<Position, Velocity>().each())
        {
            position.x += velocity.x;
            position.y += velocity.y;
            position.z += velocity.z;
        }
    }

    void Draw()
    {
        for (auto [entity, position, render] : world.registry.view<Position, Render>().each())
        {
            DrawModel(render.model, {position.x, position.y, position.z}, 1.0f, WHITE);
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

                ::std::array<void *, (int)Component::LAST> entityComponents = {};
                // helper to reduce typing and potential copy-paste errors
#define SetComponent(T) entityComponents[(int)Component::T] = world.registry.try_get<T>(entity)

                SetComponent(Position);
                SetComponent(Velocity);

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
                                case (int)Component::Position:
                                    world.registry.emplace<Position>(entity, 0.0f, 0.0f, 0.0f);
                                    break;
                                case (int)Component::Velocity:
                                    world.registry.emplace<Velocity>(entity, 0.0f, 0.0f, 0.0f);
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

                if (auto position = entityComponents[(int)Component::Position]; position)
                {
                    ImGui::InputFloat3("Position", (float *)position);
                }

                if (auto velocity = entityComponents[(int)Component::Velocity]; velocity)
                {
                    ImGui::DragFloat3("Velocity", (float *)velocity, 0.01f, -25.0f, 25.0f);

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0xcc / 255.0f, 0x24 / 255.0f, 0x1d / 255.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0xd1 / 255.0f, 0x39 / 255.0f, 0x33 / 255.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0xb7 / 255.0f, 0x20 / 255.0f, 0x1a / 255.0f, 1.0f));
                    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 30.0f);
                    if (ImGui::Button("[X]"))
                    {
                        world.registry.remove<Velocity>(entity);
                    }
                    ImGui::PopStyleColor(3);
                }

                ImGui::TreePop();
            }
        };
        world.registry.each(printEntity);
        ImGui::End();
    }
}