/******************************************************************************/
/*!
\file   AnimationComponent.hpp
\author Team PASSTA
		Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/28
\brief    Declares Animation Component

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef ANIMATION_COMPONENT_HPP
#define ANIMATION_COMPONENT_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Animation/Animation.h"
#include <native/animation.h>

#include <rsc-core/rp.hpp>
#include "ecs/internal/reflection.h"


RegisterResourceTypeForward(AnimationResourceData, "animation", unusedanimationstuff)

struct AnimationComponent
{
	rp::BasicIndexedGuid animationdata{static_cast<rp::BasicIndexedGuid>(rp::TypeNameGuid<"animation">())};
	float duration = 0.0f;
	float currentTime = 0.0f;
	float ticksPerSecond = 60.0f;
	animationState state;
	animationContainer* currentAnimationContainer = nullptr;
	blendState blend;
	bool isSkeletalAnim = false;
	bool isSpritesheetMode = false;
	animator* animatorInstance = nullptr;
	rp::Guid lastKnownAnimGuid{}; // runtime only: detects editor asset swaps vs script clip switches
};

struct AnimationBoneChannelTransformComponent {
	std::unordered_map<int, glm::mat4> bone_trans;
	bool is_active;
};

// Register animationState first 
RegisterReflectionTypeBegin(animationState, "animationState")
MemberRegistrationV<&animationState::isPlaying, "IsPlaying">,
MemberRegistrationV<&animationState::loop, "Loop">,
MemberRegistrationV<&animationState::playbackSpeed, "PlaybackSpeed">,
MemberRegistrationV<&animationState::startTime, "StartTime">,
MemberRegistrationV<&animationState::endTime, "EndTime">
RegisterReflectionTypeEnd

// Register AnimationComponent
RegisterReflectionTypeBegin(AnimationComponent, "AnimationComponent")
MemberRegistrationV<&AnimationComponent::duration, "Duration">,
MemberRegistrationV<&AnimationComponent::currentTime, "CurrentTime">,
MemberRegistrationV<&AnimationComponent::ticksPerSecond, "TicksPerSecond">,
MemberRegistrationV<&AnimationComponent::state, "State">,
MemberRegistrationV<&AnimationComponent::animationdata, "Animation">,
MemberRegistrationV<&AnimationComponent::isSpritesheetMode, "SpritesheetMode">
// Note: channel is NOT registered (runtime pointer, not serializable)
RegisterReflectionTypeEnd

#endif