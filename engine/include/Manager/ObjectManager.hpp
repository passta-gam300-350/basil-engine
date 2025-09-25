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
	void DestroyGameObject(ecs::entity obj);



};

#endif // OBJECTMANAGER_HPP