#include "Bindings/ManagedGameObjects.hpp"

#include <mono/metadata/object.h>

#include "Engine.hpp"

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


