#ifndef ANIMATION_COMPONENT_HPP
#define ANIMATION_COMPONENT_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Animation/Animation.h"

struct AnimationComponent
{
	boneChannel* channel = nullptr;
	float duration = 0.0f;
	float currentTime = 0.0f;
	float ticksPerSecond = 60.0f;
	animationState state;
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
MemberRegistrationV<&AnimationComponent::state, "State">
// Note: channel is NOT registered (runtime pointer, not serializable)
RegisterReflectionTypeEnd

#endif