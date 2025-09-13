#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "ECS/Components/TransformComponent.h"

Scene::Scene(std::string const& name)
    : m_Name(name)
{
}

Scene::~Scene() {
    // Registry will automatically clean up all entities and components
}

Entity Scene::CreateEntity(const std::string& name)
{
    // Create an entity in the registry
    entt::entity entityHandle = m_Registry.create();

    // Add required components
    auto& transform = m_Registry.emplace<TransformComponent>(entityHandle);

    // Add a name component if we have one
    if (!name.empty()) {
        // This would require a NameComponent which we haven't defined yet
        // For now, let's just assume we have it
        // m_Registry.emplace<NameComponent>(entityHandle, name);
    }

    return Entity(entityHandle, this);
}

void Scene::DestroyEntity(Entity entity)
{
    if (entity) {
        m_Registry.destroy(entity);
    }
}

void Scene::OnUpdate(float deltaTime)
{
    // Update all systems
    // Here you'd iterate over your EnTT systems and update them

    // Example:
    // m_PhysicsSystem.OnUpdate(m_Registry, deltaTime);
    // m_AnimationSystem.OnUpdate(m_Registry, deltaTime);
    // etc.
}