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

#endif