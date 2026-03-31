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
#include <unordered_map>
#include <string>
#include "Animation/Animation.h"
#include <native/animation.h>

#include <rsc-core/rp.hpp>
#include "ecs/internal/reflection.h"


RegisterResourceTypeForward(AnimationResourceData, "animation", unusedanimationstuff)

struct AnimationComponent
{
	rp::BasicIndexedGuid animationdata{static_cast<rp::BasicIndexedGuid>(rp::TypeNameGuid<"animation">())};
	// Additional animation clips available to scripts via animation.Play("name")
	// Key = friendly clip name used in scripts, Value = animation asset GUID
	std::unordered_map<std::string, rp::BasicIndexedGuid> animationClips;
	float duration = 0.0f;
	float currentTime = 0.0f;
	float ticksPerSecond = 60.0f;
	animationState state;
	std::unordered_map<std::string, rp::BasicIndexedGuid> animationClips{};
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
MemberRegistrationV<&AnimationComponent::animationClips, "AnimationClips">,
MemberRegistrationV<&AnimationComponent::animationdata, "Animation">,
MemberRegistrationV<&AnimationComponent::animationClips, "AnimationClips">,
MemberRegistrationV<&AnimationComponent::isSpritesheetMode, "SpritesheetMode">
// Note: animatorInstance/lastKnownAnimGuid are NOT registered (runtime only, not serializable)
RegisterReflectionTypeEnd

#endif
