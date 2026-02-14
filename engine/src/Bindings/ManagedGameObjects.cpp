/******************************************************************************/
/*!
\file   ManagedGameObjects.cpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3451 / UXG3450
\date   2026/02/01
\brief This file contains the definition for the ManagedGameObjects class, which
is responsible for managing game objects from managed code (C#).

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Bindings/ManagedGameObjects.hpp"

#include <mono/metadata/object.h>

#include "Engine.hpp"
#include "Render/Render.h"

uint64_t ManagedGameObjects::FindByName(MonoString* name)
{
	char* utf8Name = mono_string_to_utf8(name);

	auto entites = Engine::GetWorld();
	auto allEntities = entites.get_all_entities();

	for (auto entity : allEntities) {
		if (entity.name() == utf8Name) {
			mono_free(utf8Name);
			return entity.get_uuid();
		}
	}
	mono_free(utf8Name);
	return 0;
}

void ManagedGameObjects::DeleteGameObject(uint64_t handle)
{
	auto world = Engine::GetWorld();
	world.remove_entity(ecs::entity{ handle });

}

void ManagedGameObjects::SetVisible(uint64_t handle, mono_bool val)
{
	ecs::entity ent{ handle };
	VisibilityComponent& visibility = ent.get<VisibilityComponent>();
	visibility.m_IsVisible = (val != 0);

}

bool ManagedGameObjects::GetVisible(uint64_t handle)
{
	ecs::entity ent{ handle };
	VisibilityComponent& visibility = ent.get<VisibilityComponent>();
	return visibility.m_IsVisible;
}




