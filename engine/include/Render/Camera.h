/******************************************************************************/
/*!
\file   camera.h
\author Team PASSTA
        Chew Bangxin Steven (banxginsteven.chew@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/09
\brief    Declares the camera class

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef ENGINE_CAMERA_H
#define ENGINE_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Ecs/ecs.h"
#include "Component/Transform.hpp"
#include "Profiler/profiler.hpp"

struct CameraComponent {
    enum class CameraType : std::uint8_t {
        ORTHO,
        PERSPECTIVE
    } m_Type{ CameraType::PERSPECTIVE };
    bool m_IsActive{false};
    float m_Fov{45.f};
    float m_AspectRatio{16.f/9.f};
    float m_Near{0.1f};
    float m_Far{1000.f};
    glm::vec3 m_Up{0.f, 1.f, 0.f};
    glm::vec3 m_Right{ 1.f, 0.f, 0.f };
    glm::vec3 m_Front{ 0.f, 0.f, -1.f };
    float m_Yaw;
    float m_Pitch;
};

struct CameraSystem : public ecs::SystemBase {
public:
    // Camera struct and CameraType enum removed - game camera queried from ECS

private:
    // Cached active game camera data (updated in FixedUpdate @ 60Hz for performance)
    // Prevents ~180+ ECS queries per second from AudioSystem, C# bindings, and BVH
    struct {
        glm::vec3 position{0.f, 0.f, 0.f};
        glm::vec3 front{0.f, 0.f, -1.f};
        glm::vec3 up{0.f, 1.f, 0.f};
        glm::vec3 right{1.f, 0.f, 0.f};
        CameraComponent::CameraType type{CameraComponent::CameraType::PERSPECTIVE};
        float fov{45.f};
        float aspectRatio{16.f / 9.f};
        float nearPlane{0.1f};
        float farPlane{1000.f};
        bool isActive{false};  // false if no active camera exists in scene
    } m_CachedGameCamera;
    inline static glm::mat4 s_ViewMatrix{ 1.f };
    inline static glm::mat4 s_ProjMatrix{ 1.f };
    inline static bool s_HasCachedMatrices{ false };
    inline static glm::vec2 s_ViewportSize{ 0.f, 0.f };
    inline static glm::vec2 s_ViewportOffset{ 0.f, 0.f };

    CameraSystem(CameraSystem const&) = delete;
    CameraSystem(CameraSystem &&) = delete;
    ~CameraSystem() = default;

public:
    CameraSystem() = default;

    static CameraSystem& Instance() {
        static CameraSystem s_camera_sys{};
        return s_camera_sys;
    }

    // Returns cached game camera data (updated @ 60Hz in FixedUpdate)
    // Editor camera uses separate snapshot pattern
    // Returns struct with isActive=false if no active camera exists
    static auto const& GetActiveCameraData() {
        return Instance().m_CachedGameCamera;
    }

    // Phase 4: Removed SetActiveCamera, GetActiveCameraType, SetAuxCamera methods

    static void SetCachedMatrices(glm::mat4 const& view, glm::mat4 const& proj) {
        s_ViewMatrix = view;
        s_ProjMatrix = proj;
        s_HasCachedMatrices = true;
    }

    static void SetViewportSize(glm::vec2 viewport) {
        s_ViewportSize = viewport;
    }

    static void SetViewportOffset(glm::vec2 offset) {
        s_ViewportOffset = offset;
    }

    static glm::vec2 GetCachedViewport() {
        return s_ViewportSize;
    }

    static glm::vec2 GetCachedViewportOffset() {
        return s_ViewportOffset;
    }

    static std::pair<glm::mat4, glm::mat4> GetCachedMatrices() {
        return { s_ViewMatrix, s_ProjMatrix };
    }

    static glm::vec3 GetWorldFromScreen(glm::vec2 screenPos, glm::vec2 screenSize, float depth) {
        const auto& cam = GetActiveCameraData();
        if (!s_HasCachedMatrices) {
            return cam.position;  // Fallback to cached camera position
        }

        glm::mat4 invViewProj = glm::inverse(s_ProjMatrix * s_ViewMatrix);

        glm::vec4 ndc;
        ndc.x = (screenPos.x / screenSize.x) * 2.f - 1.f;
        ndc.y = 1.f - (screenPos.y / screenSize.y) * 2.f; // flip Y because screen origin is top-left
        ndc.z = depth * 2.f - 1.f;                        // depth expected in [0,1]
        ndc.w = 1.f;

        glm::vec4 world = invViewProj * ndc;
        world /= world.w;
        return glm::vec3(world);
	}

    static void GetRayFromScreen(glm::vec2 screenPos, glm::vec2 screenSize, glm::vec3& outOrigin, glm::vec3& outDir) {
        const auto& cam = GetActiveCameraData();
        if (!s_HasCachedMatrices) {
            outOrigin = cam.position;  // Fallback to cached camera position
            outDir = cam.front;        // Fallback to cached camera direction
            return;
        }

        glm::mat4 invViewProj = glm::inverse(s_ProjMatrix * s_ViewMatrix);

        auto ndcFromScreen = [&](float depth) {
            glm::vec4 ndc;
            ndc.x = (screenPos.x / screenSize.x) * 2.f - 1.f;
            ndc.y = 1.f - (screenPos.y / screenSize.y) * 2.f;
            ndc.z = depth * 2.f - 1.f;
            ndc.w = 1.f;
            glm::vec4 world = invViewProj * ndc;
            return world / world.w;
        };

        glm::vec4 nearP = ndcFromScreen(0.f);
        glm::vec4 farP = ndcFromScreen(1.f);

        outOrigin = glm::vec3(nearP);
        outDir = glm::normalize(glm::vec3(farP) - outOrigin);
    }

    // Phase 4: Removed GetAuxCamera method - editor camera uses snapshot pattern

    void FixedUpdate(ecs::world& w) {
        PF_SYSTEM("Camera System");
        auto& cache = Instance().m_CachedGameCamera;

        // Reset active flag - will be set to true if active camera found
        cache.isActive = false;

        auto cmp_rng = w.query_components<CameraComponent, TransformComponent>();
        for (auto [camera_comp, transform_comp] : cmp_rng) {
            if (!camera_comp.m_IsActive) {
                continue;
            }

            // Calculate camera orientation vectors from rotation
            auto quat = glm::quat(glm::radians(transform_comp.m_Rotation));
            glm::vec3 right = quat * glm::vec3(1.f, 0.f, 0.f);
            glm::vec3 up = quat * glm::vec3(0.f, 1.f, 0.f);
            glm::vec3 front = quat * glm::vec3(0.f, 0.f, 1.f);

            // Cache all game camera data for this frame
            cache.position = transform_comp.m_Translation;
            cache.front = front;
            cache.up = up;
            cache.right = right;
            cache.type = camera_comp.m_Type;
            cache.fov = camera_comp.m_Fov;
            cache.aspectRatio = camera_comp.m_AspectRatio;
            cache.nearPlane = camera_comp.m_Near;
            cache.farPlane = camera_comp.m_Far;
            cache.isActive = true;

            break;  // Only use first active camera
        }
    }
};

RegisterSystemDerivedPreUpdate(camera_system, CameraSystem, (ecs::ReadSet<CameraComponent, TransformComponent>), (ecs::EmptySet), 60)

#endif
