#include "Scene/Scene.hpp"

#include "Manager/StringManager.hpp"

Scene::Scene() : metadata{}, world{ ecs::world::new_world_instance() }
{
	
}

Scene::Scene(std::string name) : metadata{}, world{ ecs::world::new_world_instance() }
{
	metadata.uuid = UUID<128>::Generate();
	metadata.name = StringManager::GetInstance().Intern_String(std::move(name));
}

SceneMetadata const& Scene::GetMetadata() const
{
	return metadata;
}

