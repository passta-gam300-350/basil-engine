/******************************************************************************/
/*!
\file   AnimationSystem.hpp
\author Team PASSTA
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/28
\brief  Declares Animation system 

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
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