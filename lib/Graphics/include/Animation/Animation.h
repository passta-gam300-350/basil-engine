#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>

// key frame for position, will need for interpolation
struct keyFramePosition
{
	float timeStamp;
	glm::vec3 position;
};

// key frame for rotation, will need for interpolation
struct keyFrameRotation
{
	float timeStamp;
	glm::quat rotation;
};

// key frame for scaleA, will need for interpolation
struct keyFrameScale
{
	float timeStamp;
	glm::vec3 scale;
};

struct bone
{

};