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
	scn.m_slot_ct = nd["scene"]["slot"].as<std::uint32_t>();
	scn.m_guid = rp::Guid::to_guid(nd["scene"]["guid"].as<std::string>());
	YAML::Node const& deps{ nd["scene"]["dependencies"] };
	for (auto const& guid : deps) {
		rp::Guid scn_guid = rp::Guid::to_guid(guid.as<std::string>());
		scn.m_scene_dependencies.emplace_back(scn_guid);
		if (!Engine::GetSceneRegistry().IsLoaded(scn_guid)) {
			Engine::GetSceneRegistry().LoadScene(scn_guid);
		}
	}
	// Deserialize render settings (Unity-style skybox, etc.)
	if (nd["render_settings"]) {
		YAML::Node const& renderSettings = nd["render_settings"];
		if (renderSettings["skybox"]) {
			YAML::Node const& skybox = renderSettings["skybox"];

			if (skybox["enabled"])
				scn.m_renderSettings.skybox.enabled = skybox["enabled"].as<bool>();
			if (skybox["exposure"])
				scn.m_renderSettings.skybox.exposure = skybox["exposure"].as<float>();

			if (skybox["rotation"] && skybox["rotation"].size() == 3) {
				scn.m_renderSettings.skybox.rotation.x = skybox["rotation"][0].as<float>();
				scn.m_renderSettings.skybox.rotation.y = skybox["rotation"][1].as<float>();
				scn.m_renderSettings.skybox.rotation.z = skybox["rotation"][2].as<float>();
			}

			if (skybox["tint"] && skybox["tint"].size() == 3) {
				scn.m_renderSettings.skybox.tint.x = skybox["tint"][0].as<float>();
				scn.m_renderSettings.skybox.tint.y = skybox["tint"][1].as<float>();
				scn.m_renderSettings.skybox.tint.z = skybox["tint"][2].as<float>();
			}

			// Deserialize face texture GUIDs (resource pipeline)
			if (skybox["face_textures"] && skybox["face_textures"].size() == 6) {
				for (size_t i = 0; i < 6; ++i) {
					std::string guidStr = skybox["face_textures"][i].as<std::string>();
					scn.m_renderSettings.skybox.faceTextures[i] = rp::Guid::to_guid(guidStr);
				}
			}

			// Mark for reload if GUIDs are valid
			bool hasValidGuids = true;
			for (const auto& guid : scn.m_renderSettings.skybox.faceTextures) {
				if (guid == rp::null_guid) {
					hasValidGuids = false;
					break;
				}
			}
			if (hasValidGuids && scn.m_renderSettings.skybox.enabled) {
				scn.m_renderSettings.skybox.needsReload = true;
			}
		}
	}

	YAML::Node const& entities{ nd["entities"] };
	auto& reg = Engine::GetWorld().impl.get_registry();
	entt::entity ententity{};
	for (auto const& ent : entities) {
		DeserializeEntity(reg, ent, &ententity);
		ecs::entity enty = Engine::GetWorld().impl.entity_cast(ententity);
		enty.add<SceneComponent>().m_scene_guid.m_guid = scn.m_guid;
		scn.m_scene_entities.emplace(enty.get_scene_uid(), enty);
	}

	// Activate all loaded entities (required for ECS queries to find them)
	for (auto const& [sceneId, entity] : scn.m_scene_entities) {
		const_cast<ecs::entity&>(entity).add<ecs::entity::active_t>();
	}

	// Add TransformMtxComponent to all entities with TransformComponent
	auto transforms = reg.view<TransformComponent>();
	for (auto e : transforms) {
		if (!reg.all_of<TransformMtxComponent>(e)) {
			reg.emplace<TransformMtxComponent>(e);
		}
	}

	scn.m_is_dirty = true;
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

std::optional<std::reference_wrapper<Scene>> SceneRegistry::LoadSceneFromPath(std::string const& path)
{
	// Load scene using Scene::LoadYAML which includes render settings (skybox, etc.)
	auto sceneOpt = Scene::LoadYAML(path);
	if (!sceneOpt.has_value()) {
		spdlog::error("SceneRegistry: Failed to load scene from path: {}", path);
		return std::nullopt;
	}

	Scene& loadedScene = sceneOpt.value();
	rp::Guid sceneGuid = loadedScene.GetGuid();

	// Check if already loaded, if so replace it
	if (IsLoaded(sceneGuid)) {
		UnloadScene(sceneGuid);
	}

	// Add scene to registry
	m_loaded_scenes.emplace(sceneGuid, std::move(loadedScene));

	// Set as active scene
	SetActiveScene(sceneGuid);

	spdlog::info("SceneRegistry: Scene loaded from {} with GUID {}, skybox enabled: {}",
		path, sceneGuid.to_hex(), GetActiveScene().value().get().GetRenderSettings().skybox.enabled);

	return GetScene(sceneGuid);
}

void SceneRegistry::UnloadScene(rp::Guid scn_guid)
{
	auto scnres{ GetScene(scn_guid) };
	if (scnres) {
		scnres.value().get().Clear();
		m_loaded_scenes.erase(scn_guid);
	}
}
ecs::entity Scene::CreateEntity() {
	ecs::entity e = Engine::GetWorld().add_entity();
	m_is_dirty = true;
	return AddEntityToScene(e);
}

ecs::entity Scene::AddEntityToScene(ecs::entity e) {
	auto& scncomp = e.get<SceneComponent>();
	auto& scnidcomp = e.get<SceneIDComponent>();
	auto scnres = Engine::GetSceneRegistry().GetScene(scncomp.m_scene_guid.m_guid);
	if (scnres)
		scnres.value().get().m_scene_entities.erase(scnidcomp.m_scene_id);

	scncomp.m_scene_guid.m_guid = m_guid;
	scnidcomp.m_scene_id = GetNextSlot();

	m_is_dirty = true;

	m_scene_entities.emplace(scnidcomp.m_scene_id, e);
	return e;
}

void SceneRegistry::onCreateAssignToDefault(ecs::entity e) {
	auto& scncomp = e.add<SceneComponent>();
	auto& scnidcomp = e.add<SceneIDComponent>();
	auto& scn = m_loaded_scenes[rp::null_guid];
	scn.SceneName() = "Scene";

	scncomp.m_scene_guid.m_guid = rp::null_guid;
	scnidcomp.m_scene_id = scn.GetNextSlot();

	scn.Dirty() = true;

	scn.GetSceneEntitites().emplace(scnidcomp.m_scene_id, e);
}

void SceneRegistry::onCreateAssignSceneIDToDefault(ecs::entity e)
{
	auto& scncomp = e.add<SceneComponent>();
	auto& scnidcomp = e.get<SceneIDComponent>();
	auto& scn = m_loaded_scenes[rp::null_guid];
	scn.SceneName() = "Scene";
	scncomp.m_scene_guid.m_guid = rp::null_guid;

	scn.Dirty() = true;

	scn.GetSceneEntitites().emplace(scnidcomp.m_scene_id, e);
}

void SceneRegistry::onDestroySceneComponent(ecs::entity e)
{
	auto& scncomp = e.get<SceneComponent>();
	auto& scnidcomp = e.get<SceneIDComponent>();
	auto scnres = Engine::GetSceneRegistry().GetScene(scncomp.m_scene_guid.m_guid);
	if (scnres)
	{
		scnres.value().get().GetSceneEntitites().erase(scnidcomp.m_scene_id);
		scnres.value().get().Dirty() = true;
	}
}

REGISTER_RESOURCE_TYPE_ALIASE(std::optional<Scene>, scene, [](const char* data) -> std::optional<Scene> {
	SceneResourceData scnData = rp::serialization::binary_serializer::deserialize<SceneResourceData>(
		reinterpret_cast<const std::byte*>(data)
	);
	YAML::Node nd{ YAML::Load(reinterpret_cast<const char*>(scnData.scene_data.Raw())) };
	return Scene::LoadYAMLNode(nd);
	}, [](std::optional<Scene>) {})