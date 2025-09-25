#pragma once

#include "../Resources/Mesh.h"
#include "../Resources/Material.h"
#include "Light.h"
#include "AABB.h"
#include <memory>
#include <glm/glm.hpp>

// Data structures for submitting render data to the graphics library
struct RenderableData {
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material;
    glm::mat4 transform = glm::mat4(1.0f);
    bool visible = true;
    uint32_t renderLayer = 0;
};

struct SubmittedLightData {
    Light::Type type = Light::Type::Directional;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    float range = 10.0f;
    float innerCone = 30.0f;
    float outerCone = 45.0f;
    bool enabled = true;
};