#ifndef ANIMATION_SYSTEM_HPP
#define ANIMATION_SYSTEM_HPP

#include "Ecs/ecs.h"
#include "Component/Transform.hpp"
#include "Component/AnimationComponent.hpp"

struct animationSystem : public ecs::SystemBase
{
	void FixedUpdate(ecs::world& world) override;
};

RegisterSystemDerivedPreUpdate(
    animation_system,
    animationSystem,
    (ecs::ReadSet<AnimationComponent>),
    (ecs::WriteSet<TransformComponent>),
    60
)
#endif