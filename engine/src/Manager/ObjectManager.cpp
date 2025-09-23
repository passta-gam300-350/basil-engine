#include "Manager/ObjectManager.hpp"

#include "Engine.hpp"
#include "Component/IdentificationComponent.hpp"
#include "components/transform.h"

ObjectManager& ObjectManager::GetInstance()
{
	static ObjectManager instance;
	return instance;
}

ecs::entity ObjectManager::CreateGameObject()
{
	// Game object factory method
	ecs::world currentWorld = Engine::GetWorld();
	ecs::entity gameObject = currentWorld.add_entity();

	currentWorld.add_component_to_entity<IdentifierComponent>(gameObject, "DEFAULT_GO");
	currentWorld.add_component_to_entity<TransformComponent>(gameObject);

	// Add relevant components to the entity here

	IdentifierComponent& d = currentWorld.get_component_from_entity<IdentifierComponent>(gameObject);
	


	// Link with MONO here if needed

	/*
	 * 1. Create entity in mono world
	 * 2. Update mapping in mono <-> native
	 * 3. Add relevant components in mono world
	 * 4. Link the mono entity with native entity (store the native entity handle in mono component)
	 * 5. Return the native entity handle
	 * 
	 */
	return gameObject;

}
