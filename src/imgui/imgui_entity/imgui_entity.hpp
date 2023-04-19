#pragma once

#include "entity_reflection.hpp"
#include <cstdint>
#include <entt/entt.hpp>
#include <imgui.h>
#include <optional>

template<typename Derived, typename ComponentType, size_t IDValue>
class ImGuiEntity
{
  public:
    using Derived = Derived;

    ImGuiEntity()
    {
        RegisterSelf();
    }

    static void RegisterSelf()
    {
        EntityReflection::Register<ImGuiEntity<Derived, ComponentType, IDValue>>();
    }

    // Type needs to be erased or a pointer to the function can't be created
    static std::optional<void*> GetComponent(entt::registry& registry, entt::entity entity)
    {
        auto pointer = registry.try_get<ComponentType>(entity);
        if(pointer)
            return pointer;
        else
            return std::nullopt;
    }

    static bool TryViewOne(entt::registry& registry, entt::entity entity)
    {
        auto component = registry.try_get<ComponentType>(entity);
        if(component != nullptr)
            Derived::View(*component);
        return component != nullptr;
    }

    static bool TryModifyOne(entt::registry& registry, entt::entity entity, bool allowDeletion)
    {
        auto component = registry.try_get<ComponentType>(entity);
        if(component != nullptr)
            Derived::Modify(registry, entity, *component, allowDeletion);
        return component != nullptr;
    }

    static void TryDuplicate(entt::registry& registry, entt::entity source, entt::entity target)
    {
        if(std::optional<void*> component = GetComponent(registry, source); component)
            Derived::Duplicate(registry, *(ComponentType*)component.value(), target);
    }

    constexpr static uint32_t ID = IDValue;

  protected:
    static bool AddRemoveButton(const char* text, entt::registry& registry, entt::entity entity)
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

        bool remove = ImGui::Button(text);
        if(remove)
            registry.remove<ComponentType>(entity);
        ImGui::PopStyleColor(3);
        return remove;
    }
};