#ifndef SCENE_HPP
#define SCENE_HPP
#include "Handle.hpp"
#include "ecs/internal/world.h"

struct SceneMetadata
{
	UUID<128> uuid;
	uint32_t name;
	bool loaded = false;
};


class Scene
{
	
private:
	SceneMetadata metadata;
	ecs::world world;
public:
	Scene();
	Scene(std::string name);
	SceneMetadata const& GetMetadata() const;



	

};

#endif