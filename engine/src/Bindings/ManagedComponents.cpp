
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
#include "Component/VideoComponent.hpp"
#include "Input/Button.h"
#include "components/behaviour.hpp"
#include "components/transform.h"
#include "Manager/MonoEntityManager.hpp"
#include "Particles/ParticleComponent.h"
#include "Physics/Physics_Components.h"
#include "Render/Camera.h"
#include "Render/Render.h"
#include "System/Audio.hpp"
#include "System/BehaviourSystem.hpp"

std::unordered_map<uint32_t, ManagedComponents::ComponentChecker> ManagedComponents::componentTypeMap{};
std::unordered_map<uint32_t, ManagedComponents::ComponentDeletor> ManagedComponents::componentDeletorMap{};

std::unordered_map<std::string, uint32_t> ManagedComponents::componentMapped{};

void ManagedComponents::RegisterComponentType(uint32_t componentTypeId, ComponentChecker getComponentFn, ComponentDeletor deleteComponentFn)
{
	componentTypeMap[componentTypeId] = getComponentFn;
	componentDeletorMap[componentTypeId] = deleteComponentFn;
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
		}, [](ecs::entity& entity)
		{
			entity.remove<PositionComponent>();
			entity.remove<RotationComponent>();
			entity.remove<ScaleComponent>();
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
		}, [](ecs::entity& entity)
		{
			entity.remove<CameraComponent>();
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
		}, [](ecs::entity& entity)
		{
			entity.remove<RigidBodyComponent>();
		});
		return id;
	}
	if (strName == "Audio")
	{
		componentMapped[strName] = 1000000 + registerCount++; // ID for Audio component
		unsigned id = componentMapped[strName];
		RegisterComponentType(id, [](ecs::entity const& entity)
		{
			// Placeholder for Audio component check
			bool result = entity.any<AudioComponent>(); // Replace with actual Audio component
			return result;
		}, [](ecs::entity& entity)
		{
			entity.remove<AudioComponent>();
		});
		return id;
	}
	if (strName == "Video")
	{
		componentMapped[strName] = 1000000 + registerCount++; // ID for Video component
		unsigned id = componentMapped[strName];
		RegisterComponentType(id, [](ecs::entity const& entity)
		{
			// Placeholder for Video component check
			bool result = entity.any<VideoComponent>(); // Replace with actual Video component
			return result;
		}, [](ecs::entity& entity)
		{
			entity.remove<VideoComponent>();
		});
		return id;
	}
	if (strName == "MeshRenderer")
	{
		componentMapped[strName] = 1000000 + registerCount++; // ID for MeshRenderer component
		unsigned id = componentMapped[strName];
		RegisterComponentType(id, [](ecs::entity const& entity)
		{
			bool result = entity.any<MeshRendererComponent>();
			return result;
		}, [](ecs::entity& entity)
		{
			entity.remove<MeshRendererComponent>();
		});
		return id;
	}

	if (strName == "Particle")
	{
		componentMapped[strName] = 1000000 + registerCount++; // ID for Particle component
		unsigned id = componentMapped[strName];
		RegisterComponentType(id, [](ecs::entity const& entity)
		{
			// Placeholder for Particle component check
			bool result = entity.any<ParticleComponent>(); // Replace with actual Particle component
			return result;
		}, [](ecs::entity& entity)
		{
			entity.remove<ParticleComponent>();
		});
		return id;
	}
	if (strName == "HudComponent")
	{
		componentMapped[strName] = 1000000 + registerCount++; // ID for Particle component
		unsigned id = componentMapped[strName];
		RegisterComponentType(id, [](ecs::entity const& entity)
		{
			// Placeholder for Particle component check
			bool result = entity.any<HUDComponent>(); // Replace with actual Particle component
			return result;
		}, [](ecs::entity& entity)
		{
			entity.remove<HUDComponent>();
		});
		return id;
	}

	if (strName == "Button")
	{
		componentMapped[strName] = 1000000 + registerCount++;
		unsigned id = componentMapped[strName];
		RegisterComponentType(id, [](ecs::entity const& entity)
		{
			return entity.any<Button>();
		}, [](ecs::entity& entity)
		{
			entity.remove<Button>();
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

void ManagedComponents::DeleteComponent(uint64_t handle, uint32_t typeHandle)
{
	ecs::entity ent{ handle };
	
	if (typeHandle == 1079998) // Behavior component
	{
		return;
	}


	if (componentTypeMap.find(typeHandle) != componentTypeMap.end())
	{
		auto deletor = componentDeletorMap[typeHandle];
		deletor(ent);
	}
	
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



