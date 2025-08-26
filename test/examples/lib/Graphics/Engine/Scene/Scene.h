#pragma once

#include <entt/entt.hpp>
#include <string>
#include <memory>

// Forward declarations
class Entity;
class TransformSystem;
class VisibilitySystem;

class Scene
{
public:
    Scene(const std::string& name = "Untitled");
    ~Scene();

    Entity CreateEntity(const std::string& name = "Entity");
    void DestroyEntity(Entity entity);

    // Update pure ECS systems (no rendering)
    void OnUpdate(float deltaTime);

    entt::registry& GetRegistry() { return m_Registry; }
    const std::string& GetName() const { return m_Name; }

private:
    void InitializeSystems();

    entt::registry m_Registry;
    std::string m_Name;
    
    // Pure ECS systems - Scene owns these (engine logic, not rendering)
    std::unique_ptr<TransformSystem> m_TransformSystem;
    std::unique_ptr<VisibilitySystem> m_VisibilitySystem;

    friend class Entity;
    friend class SceneRenderer;
};