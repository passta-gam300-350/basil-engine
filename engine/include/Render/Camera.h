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

    static glm::vec3 GetWorldFromScreen(glm::vec2 screenPos, glm::vec2 screenSize, float depth) {
        Camera const& cam{ GetActiveCamera() };
        
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