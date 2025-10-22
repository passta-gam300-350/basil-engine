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
            glm::quat q = glm::quat(transform.m_Rotation);
            glm::mat4 rotationMatrix = glm::toMat4(q);
            mtx.m_Mtx = glm::translate(rotationMatrix * glm::scale(glm::mat4{1.f}, transform.m_Scale), transform.m_Translation);
        }
    }
};

RegisterSystemDerivedPreUpdate(transform_system, TransformSystem, (ecs::ReadSet<TransformComponent>), (ecs::WriteSet<TransformMtxComponent>), 60)

#endif