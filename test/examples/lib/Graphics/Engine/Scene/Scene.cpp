#include "Scene.h"
#include "Entity.h"
#include "../ECS/Components/TransformComponent.h"
#include "../ECS/Components/VisibilityComponent.h"
#include "../ECS/Systems/TransformSystem.h"
#include "../ECS/Systems/VisibilitySystem.h"

Scene::Scene(const std::string& name)
    : m_Name(name)
{
    InitializeSystems();
}

Scene::~Scene() {
    // Systems and registry will automatically clean up
}

Entity Scene::CreateEntity(const std::string& name)
{
    // Create an entity in the registry
    entt::entity entityHandle = m_Registry.create();

    // Add required components
    auto& transform = m_Registry.emplace<TransformComponent>(entityHandle);
    auto& visibility = m_Registry.emplace<VisibilityComponent>(entityHandle);

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
    // Update pure ECS systems (no rendering logic)
    m_TransformSystem->OnUpdate(this, deltaTime);
    m_VisibilitySystem->OnUpdate(this, deltaTime);
    
    // Future systems would go here:
    // m_PhysicsSystem->OnUpdate(this, deltaTime);
    // m_AudioSystem->OnUpdate(this, deltaTime);
    // m_AISystem->OnUpdate(this, deltaTime);
}

void Scene::InitializeSystems()
{
    // Create pure ECS systems - no rendering concerns
    m_TransformSystem = std::make_unique<TransformSystem>();
    m_VisibilitySystem = std::make_unique<VisibilitySystem>();
}