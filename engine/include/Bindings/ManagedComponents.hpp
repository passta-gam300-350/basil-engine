
/******************************************************************************/
/*!
\file   ManagedComponents.hpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the declaration for the ManagedComponents class, which
is responsible for managing and getting various components in the managed environment.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#ifndef MANAGEDCOMPONENTS_HPP
#define MANAGEDCOMPONENTS_HPP

#include <cstdint>

#include <mono/metadata/object.h>
#include "ecs/internal/entity.h"






class ManagedComponents
{
	using ComponentChecker = std::function<bool (ecs::entity& handle)>;
public:



	static std::unordered_map<uint32_t, ComponentChecker> componentTypeMap;
	static std::unordered_map<std::string, uint32_t> componentMapped;

	
	static bool HasComponent(uint64_t handle, uint32_t typeHandle);


	static uint32_t ManagedRegisterComponent(MonoString* name);

	static MonoObject* GetManagedComponent(uint64_t handle, MonoString* fullname);



	// Native Side: Registers a component type with its corresponding getter function.
	static void RegisterComponentType(uint32_t componentTypeId, ComponentChecker getComponentFn);
};





#endif // MANAGEDCOMPONENTS_HPP