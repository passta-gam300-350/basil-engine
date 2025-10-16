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
    uint32_t objectID = 0;  // Unique ID for object picking (0 = no picking)
    uint32_t modelInstanceID = 0;  // ID shared by all meshes of the same model instance
};

struct SubmittedLightData {
    Light::Type type = Light::Type::Directional;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;              // Legacy: for backward compatibility
    float diffuseIntensity = 1.0f;       // Ogldev-style: direct light intensity
    float ambientIntensity = 0.0f;       // Ogldev-style: per-light ambient contribution
    float range = 10.0f;
    float innerCone = 30.0f;
    float outerCone = 45.0f;
    bool enabled = true;
};

// Picking data structures
struct PickingResult {
    uint32_t objectID = 0;          // ID of picked object (0 = nothing picked)
    glm::vec3 worldPosition = glm::vec3(0.0f);  // 3D world position of pick
    float depth = 1.0f;             // Normalized depth value [0,1]
    bool hasHit = false;            // Whether any object was picked
};

struct MousePickingQuery {
    int screenX, screenY;           // Mouse screen coordinates
    int viewportWidth, viewportHeight;  // Viewport dimensions
};