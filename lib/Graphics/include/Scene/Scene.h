#pragma once

#include <entt/entt.hpp>
#include <string>
#include <memory>

class Entity;

class Scene
{
public:
    Scene(const std::string& name = "Untitled");
    ~Scene();

    Entity CreateEntity(const std::string& name = "Entity");
    void DestroyEntity(Entity entity);

    void OnUpdate(float deltaTime);

    entt::registry& GetRegistry() { return m_Registry; }
    const std::string& GetName() const { return m_Name; }

private:
    entt::registry m_Registry;
    std::string m_Name;

    friend class Entity;
    friend class SceneRenderer;
};