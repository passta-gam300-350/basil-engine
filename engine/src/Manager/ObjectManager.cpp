#include "Manager/ObjectManager.hpp"

#include "Engine.hpp"
#include "Component/Identification.hpp"
#include "Component/RelationshipComponent.hpp"
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

	// Add relevant components to the entity here
	
	

	// Transformation group
	gameObject.add<TransformComponent>();
	gameObject.add<PositionComponent>();
	gameObject.add<RelationshipComponent>(); // Scene graph support



	


	// Mono Initialization here

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


void ObjectManager::DestroyGameObject(ecs::entity obj)
{
	ecs::world currentWorld = Engine::GetWorld();
	currentWorld.remove_entity(obj);
}
