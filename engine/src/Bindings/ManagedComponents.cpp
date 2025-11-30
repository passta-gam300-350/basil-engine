
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

#include "ABI/CSKlass.hpp"
#include "components/behaviour.hpp"
#include "components/transform.h"
#include "Manager/MonoEntityManager.hpp"
#include "Physics/Physics_Components.h"
#include "Render/Camera.h"
#include "System/Audio.hpp"
#include "System/BehaviourSystem.hpp"

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
	if (strName == "Rigidbody")
	{
		componentMapped[strName] = 1000000 + registerCount++; // ID for Rigidbody component
		unsigned id = componentMapped[strName];
		RegisterComponentType(id, [](ecs::entity const& entity)
		{
			// Placeholder for Rigidbody component check
			bool result = entity.any<RigidBodyComponent>(); // Replace with actual Rigidbody component
			return result;
		});
		return id;
	}

	if (strName == "Audio")
	{
		componentMapped[strName] = 1000000 + registerCount++; // ID for Rigidbody component
		unsigned id = componentMapped[strName];
		RegisterComponentType(id, [](ecs::entity const& entity)
		{
			// Placeholder for Audio component check
			bool result = entity.any<AudioComponent>(); // Replace with actual Audio component
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


MonoObject* ManagedComponents::GetManagedComponent(uint64_t handle, MonoString* fullname)
{
	ecs::entity ent{ handle };

	const char* nativeName = mono_string_to_utf8(fullname);

	std::string strName = nativeName;

	behaviour& bhvr = ent.get<behaviour>();

	{
		for (auto id : bhvr.scriptIDs)
		{
			auto instance = MonoEntityManager::GetInstance().GetInstance(id);
			if (!instance)
				continue;
			auto klass = instance->Klass();

			std::string_view name = klass->Name();
			std::string_view ns = klass->Namespace();
			std::string fn{};
			if (ns.empty())
			{
				fn = name;
			}
			else
			{
				fn = std::format("{}.{}", ns, name);
			}
			if (fn == nativeName)
			{
				return instance->Object();
			}

		}
	}

	return nullptr;
}



