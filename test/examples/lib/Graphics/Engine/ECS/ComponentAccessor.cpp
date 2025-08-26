#include "ComponentAccessor.h"
#include <unordered_map>

// Thread-local storage for adapter instances to ensure thread safety
thread_local std::unordered_map<entt::entity, TransformComponentAdapter> g_TransformAdapters;
thread_local std::unordered_map<entt::entity, VisibilityComponentAdapter> g_VisibilityAdapters;

ITransformComponent* EngineComponentAccessor::GetTransform(entt::registry& registry, entt::entity entity) {
    if (!registry.all_of<TransformComponent>(entity)) {
        return nullptr;
    }
    
    auto& transform = registry.get<TransformComponent>(entity);
    
    // Create or update adapter
    auto it = g_TransformAdapters.find(entity);
    if (it == g_TransformAdapters.end()) {
        g_TransformAdapters.emplace(entity, TransformComponentAdapter(transform));
        it = g_TransformAdapters.find(entity);
    }
    
    return &it->second;
}

IVisibilityComponent* EngineComponentAccessor::GetVisibility(entt::registry& registry, entt::entity entity) {
    if (!registry.all_of<VisibilityComponent>(entity)) {
        return nullptr;
    }
    
    auto& visibility = registry.get<VisibilityComponent>(entity);
    
    // Create or update adapter
    auto it = g_VisibilityAdapters.find(entity);
    if (it == g_VisibilityAdapters.end()) {
        g_VisibilityAdapters.emplace(entity, VisibilityComponentAdapter(visibility));
        it = g_VisibilityAdapters.find(entity);
    }
    
    return &it->second;
}

bool EngineComponentAccessor::HasTransform(entt::registry& registry, entt::entity entity) {
    return registry.all_of<TransformComponent>(entity);
}

bool EngineComponentAccessor::HasVisibility(entt::registry& registry, entt::entity entity) {
    return registry.all_of<VisibilityComponent>(entity);
}