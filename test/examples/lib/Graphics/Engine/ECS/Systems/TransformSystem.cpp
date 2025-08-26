#include "TransformSystem.h"
#include "../Components/TransformComponent.h"
#include <iostream>

void TransformSystem::OnUpdate(Scene* scene, float deltaTime)
{
    if (!scene)
        return;

    auto& registry = scene->GetRegistry();

    // Update animations (if we had animation components)
    UpdateAnimations(registry, deltaTime);
    
    // Update hierarchical transforms (if we had parent/child relationships)
    UpdateTransformHierarchies(registry);
    
    // This is PURE ECS logic - no rendering, no cameras, no OpenGL
}

void TransformSystem::UpdateAnimations(entt::registry& registry, float deltaTime)
{
    // Example: If we had AnimationComponent, we'd update it here
    // auto view = registry.view<TransformComponent, AnimationComponent>();
    // view.each([deltaTime](auto entity, auto& transform, auto& animation) {
    //     // Update transform based on animation
    // });
    
    // For now, this is empty since we don't have animation components yet
}

void TransformSystem::UpdateTransformHierarchies(entt::registry& registry)
{
    // Example: If we had ParentComponent/ChildComponent, we'd update hierarchies here
    // auto view = registry.view<TransformComponent, ParentComponent>();
    // view.each([&registry](auto entity, auto& transform, auto& parent) {
    //     // Update child transforms based on parent transforms
    // });
    
    // For now, this is empty since we don't have hierarchy components yet
}