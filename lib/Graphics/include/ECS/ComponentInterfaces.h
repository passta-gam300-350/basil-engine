#pragma once

#include <glm/glm.hpp>
#include <entt/entt.hpp>

// EnTT is now included - no forward declarations needed

// Abstract interfaces to decouple Graphics from Engine components
// These define the contract without depending on concrete implementations

class ITransformComponent {
public:
    virtual ~ITransformComponent() = default;
    virtual glm::mat4 GetTransform() const = 0;
    virtual glm::vec3 GetPosition() const = 0;
    virtual glm::vec3 GetRotation() const = 0;
    virtual glm::vec3 GetScale() const = 0;
};

class IVisibilityComponent {
public:
    virtual ~IVisibilityComponent() = default;
    virtual bool IsVisible() const = 0;
    virtual void SetVisible(bool visible) = 0;
};

// Component accessor interface for decoupled ECS access
class IComponentAccessor {
public:
    virtual ~IComponentAccessor() = default;
    
    // Type-erased component access
    virtual ITransformComponent* GetTransform(entt::registry& registry, entt::entity entity) = 0;
    virtual IVisibilityComponent* GetVisibility(entt::registry& registry, entt::entity entity) = 0;
    
    // Component existence checks
    virtual bool HasTransform(entt::registry& registry, entt::entity entity) = 0;
    virtual bool HasVisibility(entt::registry& registry, entt::entity entity) = 0;
};

// Utility functions for working with component interfaces
namespace ComponentUtils {
    // Global component accessor (set by Engine)
    void SetComponentAccessor(IComponentAccessor* accessor);
    IComponentAccessor* GetComponentAccessor();
}