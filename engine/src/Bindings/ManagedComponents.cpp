
/******************************************************************************/
/*!
\file   ManagedComponents.cpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the implementation for the ManagedComponents class, which
is responsible for managing and getting various components in the managed environment.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Bindings/ManagedComponents.hpp"

#include <iostream>

#include "components/transform.h"
#include "Manager/MonoEntityManager.hpp"
#include "Render/Camera.h"

std::unordered_map<uint32_t, ManagedComponents::ComponentChecker> ManagedComponents::componentTypeMap{};
std::unordered_map<std::string, uint32_t> ManagedComponents::componentMapped{};

void ManagedComponents::RegisterComponentType(uint32_t componentTypeId, ComponentChecker getComponentFn)
{
	componentTypeMap[componentTypeId] = getComponentFn;
}

uint32_t ManagedComponents::ManagedRegisterComponent(MonoString* name)
{
	static int registerCount = 1;
	const char* nativeName = mono_string_to_utf8(name);
	std::string strName = nativeName;

	if (componentMapped.contains(strName))
		return componentMapped[strName];
	

	if (strName == "Transform")
	{
		componentMapped[strName] = 1000000 + registerCount++; // ID for Transform component
		unsigned id = componentMapped[strName];

		RegisterComponentType(id, [](ecs::entity const& entity)
		{
			bool result = entity.any<PositionComponent>();
			return result;
		});

		return id;
	}

	if (strName == "Camera")
	{
		componentMapped[strName] = 1000000 + registerCount++; // ID for Camera component
		unsigned id = componentMapped[strName];
		RegisterComponentType(id, [](ecs::entity const& entity)
		{
			// Placeholder for Camera component check
			bool result = entity.any<CameraComponent>(); // Replace with actual CameraComponent
			return result;
		});
		return id;
	}

	

	return 0;
}


bool ManagedComponents::HasComponent(uint64_t handle, uint32_t typeHandle)
{
	ecs::entity ent{ handle };

	if (typeHandle == 1079998) // Behavior component
	{
		// Placeholder logic for Behavior component
		return ent.any<PositionComponent>(); // Replace with actual Behavior component check
	}


	if (componentTypeMap.find(typeHandle) != componentTypeMap.end())
	{
		auto checker = componentTypeMap[typeHandle];
		return checker(ent);
	}
	return false;
}



