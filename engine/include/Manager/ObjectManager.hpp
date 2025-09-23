#ifndef OBJECTMANAGER_HPP
#define OBJECTMANAGER_HPP
#include "ecs/internal/entity.h"

class ObjectManager
{

public:
	static ObjectManager& GetInstance();
	void init();
	void shutdown();

	ecs::entity CreateGameObject();



};

#endif // OBJECTMANAGER_HPP