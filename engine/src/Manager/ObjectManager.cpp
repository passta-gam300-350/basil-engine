#include "Manager/ObjectManager.hpp"

#include "Engine.hpp"
#include "Component/Identification.hpp"
#include "Component/RelationshipComponent.hpp"
#include "components/transform.h"
#include <Manager/MonoEntityManager.hpp>
#include <spdlog/spdlog.h>

ObjectManager& ObjectManager::GetInstance()
{
	static ObjectManager instance;
	return instance;
}

ecs::entity ObjectManager::CreateGameObject(glm::vec3 pos, glm::vec3 scale, glm::vec3 rot)
{
	// Game object factory method
	ecs::world currentWorld = Engine::GetWorld();
	ecs::entity gameObject = currentWorld.add_entity();

	// Add relevant components to the entity here
	
	

	// Transformation group
	gameObject.add<PositionComponent>(pos);
	gameObject.add<ScaleComponent>(scale);
	gameObject.add<RotationComponent>(rot);

	uint64_t handle = gameObject.get_uuid();
	void* args[] = { &handle };
	// add CS Managed component
	Resource::Guid id = MonoEntityManager::GetInstance().AddInstance("GameObject", "BasilEngine", args,true);

	spdlog::info("Created Mono Instance with GUID: {}", id.to_hex());




	


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
