#ifndef SCENE_HPP
#define SCENE_HPP
#include "Handle.hpp"
#include "ecs/internal/world.h"

struct SceneMetadata
{
	std::string name;
	UUID<128> uuid;
	bool loaded = false;
};


class Scene
{
	

	SceneMetadata metadata;
	ecs::world world;


};

#endif