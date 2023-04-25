#pragma once

#include "entity_reflection.hpp"
#include <cstdint>
#include <entt/entt.hpp>
#include <imgui.h>
#include <lua/lua_validator.hpp>
#include <optional>
#include <string>
#include <vector>

// There macros can be used to avoid having to change multiple placed when creating new component
// reflections
#define EntityReflectionStruct2(name)                 \
    static const char name##ReflectionName[] = #name; \
    struct name##Reflection                           \
        : public ReflectionComponent<name##Reflection, Component::name, name##ReflectionName>
#define EntityReflectionStructTail2(name) static name##Reflection Imgui##name##Instance{};

// This weirdness with the "2" functions is required if a macro should be able to take another macro
// as a parameter, and expand the parameter to the macro's actual value. For example:
//   #define SomeMacro HereIsAValue
//   #EntityReflectionStruct(SomeMacro)
// Will expand to
//   #EntityReflectionStruct2(HereIsAValue)
#define EntityReflectionStruct(name) EntityReflectionStruct2(name)
#define EntityReflectionStructTail(name) EntityReflectionStructTail2(name)

template<typename Derived, typename ComponentType, const char* name>
class ReflectionComponent
{
  public:
    using Derived = Derived;
    static constexpr const char* NAME = name;

    ReflectionComponent()
    {
        RegisterSelf();
    }

    static void RegisterSelf()
    {
        EntityReflection::Register<ReflectionComponent<Derived, ComponentType, name>>();
    }

    static void RemoveComponent(entt::registry& registry, entt::entity entity)
    {
        registry.remove<ComponentType>(entity);
    }

    // Type needs to be erased or a pointer to the function can't be created
    static std::optional<void*> GetComponent(entt::registry& registry, entt::entity entity)
    {
        if constexpr(std::is_empty_v<ComponentType>)
        {
            auto hasComponent = registry.any_of<ComponentType>(entity);
            if(hasComponent)
                return std::optional(nullptr);
            else
                return std::nullopt;
        }
        else
        {
            auto pointer = registry.try_get<ComponentType>(entity);
            if(pointer)
                return pointer;
            else
                return std::nullopt;
        }
    }

    static bool TryViewOne(entt::registry& registry, entt::entity entity)
    {
        if constexpr(std::is_empty_v<ComponentType>)
        {
            auto hasComponent = registry.any_of<ComponentType>(entity);
            if(hasComponent)
                Derived::View();
            return hasComponent;
        }
        else
        {
            auto component = registry.try_get<ComponentType>(entity);
            if(component != nullptr)
                Derived::View(*component);
            return component != nullptr;
        }
    }

    static bool TryModifyOne(entt::registry& registry, entt::entity entity, bool allowDeletion)
    {
        if constexpr(std::is_empty_v<ComponentType>)
        {
            auto hasComponent = registry.any_of<ComponentType>(entity);
            if(hasComponent)
                Derived::Modify(registry, entity, allowDeletion);
            return hasComponent;
        }
        else
        {
            auto component = registry.try_get<ComponentType>(entity);
            if(component != nullptr)
                Derived::Modify(registry, entity, *component, allowDeletion);
            return component != nullptr;
        }
    }

    static void TryDuplicate(entt::registry& registry, entt::entity source, entt::entity target)
    {
        if constexpr(std::is_empty_v<ComponentType>)
        {
            if(std::optional<void*> component = GetComponent(registry, source); component)
                Derived::Duplicate(registry, target);
        }
        else
        {
            if(std::optional<void*> component = GetComponent(registry, source); component)
                Derived::Duplicate(registry, *(ComponentType*)component.value(), target);
        }
    }

    [[nodiscard]] static bool CreateFromLua(
        lua_State* lua,
        entt::registry& registry,
        entt::entity entity)
    {
        std::optional<std::string> errorString = Derived::GetLuaValidator(lua).GetErrorString();

        if(errorString.has_value())
        {
            lua_pushstring(lua, errorString.value().c_str());
            return false;
        }

        Derived::CreateFromLuaInternal(lua, registry, entity);
        return true;
    }

    // constexpr static uint32_t ID = IDValue;

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