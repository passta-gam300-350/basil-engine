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
#include "Component/SkeletonComponent.hpp"

struct animationSystem : public ecs::SystemBase
{
	void FixedUpdate(ecs::world& world) override;
    void Init() override;
    void Exit() override;
};

void InitializeSkeletalAnimation(AnimationComponent& animComp, SkeletonComponent& skelComp, const skeleton& skeletonData, animationContainer* animation);

void CleanupSkeletalAnimation(AnimationComponent& animComp);

RegisterSystemDerivedPreUpdate(
    animation_system,
    animationSystem,
    (ecs::ReadSet<AnimationComponent>),
    (ecs::WriteSet<TransformComponent, SkeletonComponent>),
    60
)

#endif