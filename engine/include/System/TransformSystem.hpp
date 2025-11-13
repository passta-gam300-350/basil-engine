/******************************************************************************/
/*!
\file   transformsystem.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/09
\brief    Declares the scenegraph class

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef TRANSFORM_SYSTEM_H
#define TRANSFORM_SYSTEM_H

#include "Ecs/ecs.h"
#include "Component/Transform.hpp"
#include <glm/ext.hpp>

struct TransformSystem : public ecs::SystemBase {
public:
    TransformSystem() = default;

    void FixedUpdate(ecs::world& w) {
        auto transforms = w.query_components<TransformComponent, TransformMtxComponent>();
        for (auto [transform, mtx] : transforms) {
            // Build TRS matrix: Translation * Rotation * Scale
            // m_Rotation is in degrees, must convert to radians for quaternion
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), transform.m_Translation);
            glm::mat4 rotation = glm::mat4_cast(glm::quat(glm::radians(transform.m_Rotation)));
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), transform.m_Scale);
            mtx.m_Mtx = translation * rotation * scale;  // T * R * S (correct order)
        }
    }
};

RegisterSystemDerivedPreUpdate(transform_system, TransformSystem, (ecs::ReadSet<TransformComponent>), (ecs::WriteSet<TransformMtxComponent>), 60)

#endif