#ifndef ENGINE_RENDER_H
#define ENGINE_RENDER_H

#include <memory>
#include <Core/Renderer.h>
#include <Scene/SceneRenderer.h>
#include <Utility/Camera.h>
#include <serialisation/guid.h>
#include "Manager/ResourceSystem.hpp"
#include <native/mesh.h>

#include "System/System.hpp"

struct MeshRendererComponent {
    Resource::Guid m_MeshGuid;
    Resource::Guid m_MaterialGuid;
};

struct ResourceTypeMesh : public BasicResource {
    std::shared_ptr<Mesh> m_ptr;
    ResourceTypeMesh() = default;
    ResourceTypeMesh(Resource::MeshResource const& mr);
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

struct RenderSystem {
    // graphics lib objects
    std::unique_ptr<Renderer> m_Renderer;
    std::unique_ptr<SceneRenderer> m_SceneRenderer;
    std::unique_ptr<Camera> m_Camera;
    ResourceManager* m_ResourceManager;

    // Scene objects
    std::vector<RenderableData> m_SceneObjects;
    std::vector<SubmittedLightData> m_SceneLights;

private:
    // singleton
    static std::unique_ptr<RenderSystem> s_Instance;

public:
    static RenderSystem& Instance();
    static void NewInstance();
    //~RenderSystem() = default;

    void Init();
    void Update(ecs::world&);
    void FixedUpdate(ecs::world&);
    void Exit();
};

#endif