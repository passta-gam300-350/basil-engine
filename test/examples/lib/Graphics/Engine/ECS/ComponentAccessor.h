#pragma once

#include <ECS/ComponentInterfaces.h>
#include "Components/TransformComponent.h"
#include "Components/VisibilityComponent.h"

// Concrete implementation of component accessor for Engine components
class EngineComponentAccessor : public IComponentAccessor {
public:
    // Type-erased component access
    ITransformComponent* GetTransform(entt::registry& registry, entt::entity entity) override;
    IVisibilityComponent* GetVisibility(entt::registry& registry, entt::entity entity) override;
    
    // Component existence checks
    bool HasTransform(entt::registry& registry, entt::entity entity) override;
    bool HasVisibility(entt::registry& registry, entt::entity entity) override;
};

// Adapter classes to wrap Engine components with Graphics interfaces
class TransformComponentAdapter : public ITransformComponent {
public:
    TransformComponentAdapter(TransformComponent& transform) : m_Transform(transform) {}
    
    glm::mat4 GetTransform() const override { return m_Transform.GetTransform(); }
    glm::vec3 GetPosition() const override { return m_Transform.Translation; }
    glm::vec3 GetRotation() const override { return m_Transform.Rotation; }
    glm::vec3 GetScale() const override { return m_Transform.Scale; }
    
private:
    TransformComponent& m_Transform;
};

class VisibilityComponentAdapter : public IVisibilityComponent {
public:
    VisibilityComponentAdapter(VisibilityComponent& visibility) : m_Visibility(visibility) {}
    
    bool IsVisible() const override { return m_Visibility.IsVisible; }
    void SetVisible(bool visible) override { m_Visibility.IsVisible = visible; }
    
private:
    VisibilityComponent& m_Visibility;
};