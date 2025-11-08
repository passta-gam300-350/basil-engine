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




	// Native Side: Registers a component type with its corresponding getter function.
	static void RegisterComponentType(uint32_t componentTypeId, ComponentChecker getComponentFn);
};





#endif // MANAGEDCOMPONENTS_HPP