#ifndef ENGINE_RENDER_H
#define ENGINE_RENDER_H
#pragma once

#include <memory>
#include <Scene/SceneRenderer.h>
#include <Utility/Camera.h>
#include <serialisation/guid.h>
#include "Manager/ResourceSystem.hpp"
#include <native/mesh.h>

#include "Ecs/ecs.h"

struct MeshRendererComponent {
    Resource::Guid m_MeshGuid;
    Resource::Guid m_MaterialGuid;
};

struct VisibilityComponent{
    bool m_IsVisible;
};

struct LightComponent {
    Light::Type m_Type;
    glm::vec3 m_Direction;
    glm::vec3 m_Color;
    float m_Intensity;
    float m_Range;
    float m_InnerCone;
    float m_OuterCone;
    bool m_IsEnabled;
};

struct CameraComponent {
    enum class CameraType : std::uint8_t {
        ORTHO,
        PERSPECTIVE
    } m_Type;
    bool m_IsActive;
    float m_Fov;
    float m_AspectRatio;
    float m_Near;
    float m_Far;
    glm::vec3 m_Up;
    glm::vec3 m_Right;
};

struct RenderSystem : public ecs::SystemBase {
public:
    // Editor resource registration for in-memory assets
    static void RegisterEditorMesh(Resource::Guid guid, std::shared_ptr<Mesh> mesh);
    static void RegisterEditorMaterial(Resource::Guid guid, std::shared_ptr<Material> material);
    struct InstanceData {
        // graphics lib objects
        std::unique_ptr<SceneRenderer> m_SceneRenderer;
        std::unique_ptr<Camera> m_Camera;
        
        void Acquire();
        void Release();
    };

private:
    // singleton lazily initialised
    static std::unique_ptr<InstanceData>& InstancePtr();

public:
    static InstanceData& Instance();
    static RenderSystem System();
    //~RenderSystem() = default;

    void Init();
    void Update(ecs::world&);
    void FixedUpdate(ecs::world&);
    void Exit();
};

#endif