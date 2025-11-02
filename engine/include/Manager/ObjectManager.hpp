/******************************************************************************/
/*!
\file   ObjectManager.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contains the declaration for the ObjectManager class, which
is responsible for managing game objects within the engine.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
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