#pragma once

#include <entt/entt.hpp>
#include "../../Scene/Scene.h"
#include "../../Utility/Camera.h"

class CullingSystem
{
public:
    void OnUpdate(Scene* scene, const Camera& camera);

private:
    bool IsEntityVisible(const Camera& camera, const glm::vec3& position, float boundingSphereRadius);
};