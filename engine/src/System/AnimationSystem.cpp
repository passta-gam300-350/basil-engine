/******************************************************************************/
/*!
\file   AnimationSystem.cpp
\author Team PASSTA
		Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/28
\brief  Defines Animation system

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "System/AnimationSystem.hpp"
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Engine.hpp"
#include "Component/AnimationComponent.hpp"
void animationSystem::FixedUpdate(ecs::world& world)
{
	float dt = Engine::GetDeltaTime();
	auto animated = world.filter_entities<AnimationComponent, TransformComponent>();
	for (auto eachAEntity : animated)
	{
		auto& animationComponent = eachAEntity.get<AnimationComponent>();
		auto& transformComponent = eachAEntity.get<TransformComponent>();
		if (!animationComponent.state.isPlaying)
		{
			continue;
		}
		if (!animationComponent.channel)
		{
			continue;
		}
		if (animationComponent.duration <= 0.0f)
		{
			continue;
		}
		animationComponent.currentTime += animationComponent.ticksPerSecond * dt * animationComponent.state.playbackSpeed;
		if (animationComponent.currentTime > animationComponent.duration)
		{
			if (animationComponent.state.loop == true)
			{
				animationComponent.currentTime = fmod(animationComponent.currentTime, animationComponent.duration);
			}
			else
			{
				animationComponent.currentTime = animationComponent.duration;
				animationComponent.state.isPlaying = false;
			}
		}
		animationComponent.channel->update(animationComponent.currentTime);
		glm::mat4 localMatrix = animationComponent.channel->getLocalTransform();
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(localMatrix, scale, rotation, translation, skew, perspective);
		transformComponent.m_Translation = translation;
		transformComponent.m_Rotation = glm::degrees(glm::eulerAngles(rotation));
		transformComponent.m_Scale = scale;
		transformComponent.isDirty = true;
	}
}
