#include "Scene/Scene.hpp"
#include <native/scene.h>
#include "Manager/ResourceSystem.hpp"

std::optional<Scene> Scene::LoadYAML(std::string const& path) {
	YAML::Node nd{ YAML::LoadFile(path) };
	return LoadYAMLNode(nd);
}

std::optional<Scene> Scene::LoadYAMLNode(YAML::Node const& nd) {
	if (nd["scene"]["version"].as<std::string>() != std::string(CurrentSceneVersion)) {
		return std::nullopt; //version unsupported
	}
	Scene scn;
	scn.m_name = nd["scene"]["name"].as<std::string>();
	YAML::Node const& deps{ nd["scene"]["dependencies"] };
	for (auto const& guid : deps) {
		rp::Guid scn_guid = rp::Guid::to_guid(guid.as<std::string>());
		scn.m_scene_dependencies.emplace_back(scn_guid);
		if (!Engine::GetSceneRegistry().IsLoaded(scn_guid)) {
			Engine::GetSceneRegistry().LoadScene(scn_guid);
		}
	}
	YAML::Node const& entities{ nd["entities"] };
	auto& reg = Engine::GetWorld().impl.get_registry();
	entt::entity ententity{};
	for (auto const& ent : entities) {
		DeserializeEntity(reg, ent, &ententity);
		ecs::entity enty = Engine::GetWorld().impl.entity_cast(ententity);
		scn.m_scene_entities.emplace(enty.get_scene_uid(), enty);
	}
	return std::make_optional(scn);
}

std::optional<std::reference_wrapper<Scene>> SceneRegistry::LoadScene(rp::Guid scn_guid)
{
	auto scnres{ GetScene(scn_guid) };
	if (scnres)
		return scnres;
	auto scnloadres{ ResourceRegistry::Instance().Get<std::optional<Scene>>(scn_guid) };
	if (scnloadres) {
		m_loaded_scenes.emplace(scn_guid, scnloadres->value());
	}
	return GetScene(scn_guid);
}

void SceneRegistry::UnloadScene(rp::Guid scn_guid)
{
	auto scnres{ GetScene(scn_guid) };
	if (scnres) {
		scnres.value().get().Clear();
		m_loaded_scenes.erase(scn_guid);
	}
}

REGISTER_RESOURCE_TYPE_ALIASE(std::optional<Scene>, scene, [](const char* data) -> std::optional<Scene> {
	SceneResourceData scnData = rp::serialization::binary_serializer::deserialize<SceneResourceData>(
		reinterpret_cast<const std::byte*>(data)
	);
	YAML::Node nd{ YAML::Load(reinterpret_cast<const char*>(scnData.scene_data.Raw())) };
	return Scene::LoadYAMLNode(nd);
	}, [](std::optional<Scene>) {})