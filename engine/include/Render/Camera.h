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
    struct Camera : public CameraComponent {
        glm::vec3 m_Pos;
    };
    enum class CameraType : std::uint8_t {
        MAIN_CAMERA_ENTITY,
        AUX
    };

private:
    std::shared_ptr<Camera> m_AuxCamera{nullptr};
    Camera m_MainCamera{};
    bool m_UseAux{false};
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

    static Camera const& GetActiveCamera() {
        CameraSystem& sys{ Instance() };
        return sys.m_UseAux ? *sys.m_AuxCamera : sys.m_MainCamera;
    }

    static void SetActiveCamera(CameraType ctype) {
        Instance().m_UseAux = ctype == CameraType::AUX;
    }

    static CameraType GetActiveCameraType() {
        return Instance().m_UseAux ? CameraType::AUX : CameraType::MAIN_CAMERA_ENTITY;
    }

    static void SetAuxCamera(std::shared_ptr<Camera> sptr) {
        Instance().m_AuxCamera = sptr;
    }

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
        Camera const& cam{ GetActiveCamera() };
        if (!s_HasCachedMatrices) {
            return cam.m_Pos;
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
        Camera const& cam{ GetActiveCamera() };
        if (!s_HasCachedMatrices) {
            outOrigin = cam.m_Pos;
            outDir = cam.m_Front;
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

    static std::shared_ptr<Camera> GetAuxCamera() {
        return Instance().m_AuxCamera;
    }

    void FixedUpdate(ecs::world& w) {
        auto& [aux_cam, main_cam, is_aux] {Instance()};
       
        auto cmp_rng{w.query_components<CameraComponent, TransformComponent>()};
        for (auto [camera_comp, transform_comp] : cmp_rng) {
            if (!camera_comp.m_IsActive) {
                continue;
            }
            //update camera comp
            auto quat = glm::quat(glm::radians(transform_comp.m_Rotation));
            camera_comp.m_Right = quat * glm::vec3(1.f, 0.f, 0.f);
            camera_comp.m_Up = quat * glm::vec3(0.f, 1.f, 0.f);
            camera_comp.m_Front = quat * glm::vec3(0.f, 0.f, 1.f);
            //update Graphics camera
            static_cast<CameraComponent&>(main_cam) = camera_comp;
            main_cam.m_Pos = transform_comp.m_Translation;
            break;
        }
    }
};

RegisterSystemDerivedPreUpdate(camera_system, CameraSystem, (ecs::ReadSet<CameraComponent, TransformComponent>), (ecs::EmptySet), 60)

#endif
