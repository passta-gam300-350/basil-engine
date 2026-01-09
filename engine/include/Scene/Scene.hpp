/******************************************************************************/
/*!
\file   scene.hpp
\author Team PASSTA
		Chew Bangxin Steven (banxginsteven.chew@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/09
\brief    Declares the scene class

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef ENG_SCENE_HPP
#define ENG_SCENE_HPP

#include "ecs/ecs.h"
#include <glm/glm.hpp>
#include "Engine.hpp"
#include <rsc-core/rp.hpp>

namespace {
	static constexpr const char CurrentSceneVersion[] = "v0.0.1";
}

using SceneEntityID = std::uint32_t;

struct SceneIDComponent {
	std::uint32_t m_scene_id;
};

// Scene-level rendering settings
struct SceneRenderSettings {
	struct SkyboxSettings {
		// Unity-style: Skybox uses 6 texture assets loaded through resource pipeline
		// Order: +X (right), -X (left), +Y (top), -Y (bottom), +Z (back), -Z (front)
		std::array<rp::Guid, 6> faceTextures = {
			rp::null_guid, rp::null_guid, rp::null_guid,
			rp::null_guid, rp::null_guid, rp::null_guid
		};

		bool enabled = false;                 // Enable/disable skybox
		float exposure = 1.0f;                // HDR exposure multiplier (0.0 - 10.0)
		glm::vec3 rotation = glm::vec3(0.0f); // Euler angles (XYZ rotation in degrees)
		glm::vec3 tint = glm::vec3(1.0f);     // Color tint (RGB, 0.0 - 1.0)

		// Runtime cache (not serialized) - generated cubemap texture ID
		mutable unsigned int cachedCubemapID = 0;
		mutable bool needsReload = true; // Flag to trigger cubemap reload when GUIDs change
	} skybox;

	glm::vec3 ambientLight = glm::vec3(0.1f);     // Ambient light color
	glm::vec4 backgroundColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f); // Background clear color
};

struct SceneComponent {
	rp::BasicIndexedGuid m_scene_guid{};
};

struct SceneEntityReference {
	rp::BasicIndexedGuid m_scene_guid{};
	std::uint32_t m_scene_id;
};

struct Scene
{
	Scene() = default;
	inline void Clear() {
		auto tmp = m_scene_entities;
		for (auto& [ssid, ss] : tmp) {
			ss.destroy();
		}
		m_scene_entities.clear();
	}
	void Unload() {
		Clear();
	}

	static std::optional<Scene> LoadYAML(std::string const& path);
	static std::optional<Scene> LoadYAMLNode(YAML::Node const& nd);

	inline std::unordered_map<std::uint32_t, ecs::entity>& GetSceneEntitites() {
		return m_scene_entities;
	}

	inline std::optional<ecs::entity> GetSceneEntity(std::uint32_t sid) {
		auto it = m_scene_entities.find(sid);
		return it != m_scene_entities.end() ? std::make_optional(it->second) : std::nullopt;
	}

	ecs::entity CreateEntity();

	ecs::entity AddEntityToScene(ecs::entity e);

	inline std::string const& SceneName() const {
		return m_name;
	}

	inline std::string& SceneName() {
		return m_name;
	}

	inline std::string const& SceneDependencies() const {
		return m_name;
	}

	inline void SerializeBinaryFile(std::string const& filename);

	inline void SerializeBinaryBlob();

	inline void EnableScene() {
		for (auto& [scn_uid, entity] : m_scene_entities) {
			entity.enable();
		}
	}

	inline void DisableScene() {
		for (auto& [scn_uid, entity] : m_scene_entities) {
			entity.disable();
		}
	}

	inline void SerializeYaml(std::string const& path) {
		YAML::Node root{  };
		root["scene"]["name"] = m_name;
		root["scene"]["version"] = std::string(CurrentSceneVersion);
		root["scene"]["slot"] = m_slot_ct;
		root["scene"]["guid"] = m_guid.to_hex();
		YAML::Node deps{};
		for (auto const& guid : m_scene_dependencies) {
			deps.push_back(guid.to_hex());
		}
		root["scene"]["dependencies"] = deps;

		// Serialize render settings (Unity-style skybox, etc.)
		YAML::Node renderSettings;
		YAML::Node skybox;
		skybox["enabled"] = m_renderSettings.skybox.enabled;
		skybox["exposure"] = m_renderSettings.skybox.exposure;
		YAML::Node rotation;
		rotation.push_back(m_renderSettings.skybox.rotation.x);
		rotation.push_back(m_renderSettings.skybox.rotation.y);
		rotation.push_back(m_renderSettings.skybox.rotation.z);
		skybox["rotation"] = rotation;
		YAML::Node tint;
		tint.push_back(m_renderSettings.skybox.tint.x);
		tint.push_back(m_renderSettings.skybox.tint.y);
		tint.push_back(m_renderSettings.skybox.tint.z);
		skybox["tint"] = tint;
		// Serialize face texture GUIDs (resource pipeline)
		YAML::Node faceTextures;
		for (const auto& guid : m_renderSettings.skybox.faceTextures) {
			faceTextures.push_back(guid.to_hex());
		}
		skybox["face_textures"] = faceTextures;
		renderSettings["skybox"] = skybox;
		root["render_settings"] = renderSettings;

		entt::registry& reg{ ecs::world(m_scene_entities.begin()->second.get_world_handle()).impl.get_registry() };
		for (auto const& [scn_uid, entity] : m_scene_entities) {
			root["entities"].push_back(SerializeEntity<YAML::Node>(reg, static_cast<entt::entity>(entity.get_uid())));
		}
		std::ofstream outp{ path };
		if (outp)
			outp << root;
	}

	inline std::uint32_t GetNextSlot() {
		assert(m_scene_entities.size() < std::numeric_limits<std::uint32_t>::max() && "too many entities, no available slots!");
		while (m_scene_entities.find(m_slot_ct) != m_scene_entities.end())
			m_slot_ct++;
		return m_slot_ct++;
	}

	bool IsStale() {
		return !m_is_dirty;
	}
	bool& Dirty() {
		return m_is_dirty;
	}

	inline SceneRenderSettings& GetRenderSettings() {
		return m_renderSettings;
	}

	inline const SceneRenderSettings& GetRenderSettings() const {
		return m_renderSettings;
	}

	inline rp::Guid GetGuid() const {
		return m_guid;
	}

private:
	rp::Guid m_guid{};
	std::string m_name;
	std::unordered_map<std::uint32_t, ecs::entity> m_scene_entities;
	std::uint32_t m_slot_ct{};
	std::vector<rp::Guid> m_scene_dependencies;
	bool m_is_dirty;
	SceneRenderSettings m_renderSettings;


	
};

struct SceneRegistry{
private:
	std::unordered_map<rp::Guid, Scene> m_loaded_scenes;
	rp::Guid m_active_scene_guid; // Currently active scene for rendering/editing
	std::string m_SceneWorkingDir;
	std::vector<std::string> m_scene_order;

	bool scene_change_by_index = false;
	bool scene_change_by_file = false;

	uint32_t requested_index = -1;
	std::string requested_file = "";



public:
	SceneRegistry() = default;
	~SceneRegistry() {
		//Clear();
	}

	inline bool IsLoaded(rp::Guid scn_guid) {
		return m_loaded_scenes.find(scn_guid) != m_loaded_scenes.end();
	}


	void RequestSceneChange(uint32_t index)
	{
		scene_change_by_index = true;
		requested_index = index;

		
		
	}

	void RequestSceneChange(std::string const& path)
	{
		scene_change_by_file = true;
		requested_file = path;
		
	}

	void PollRequestSceneChange();
	std::optional<std::reference_wrapper<Scene>> LoadScene(rp::Guid scn_guid);
	std::optional<std::reference_wrapper<Scene>> LoadSceneFromPath(std::string const& path);
	std::optional<std::reference_wrapper<Scene>> LoadSceneByIndex(unsigned index);

	void GenerateManifest(std::vector<std::string>& scene_order);
	void ReadManifest(std::string path);
	void GetManifest(std::vector<std::string>& manifest);
	void UnloadScene(rp::Guid scn_guid);
	inline std::optional<std::reference_wrapper<Scene>> GetScene(rp::Guid scn_guid) {
		auto it{ m_loaded_scenes.find(scn_guid) };
		return it != m_loaded_scenes.end() ? std::make_optional(std::ref(it->second)) : std::nullopt;
	}
	inline std::unordered_map<rp::Guid, Scene> GetAllScenes() {
		return m_loaded_scenes;
	}

	// Active scene management (Unity-style)
	inline void SetActiveScene(rp::Guid scn_guid) {
		if (IsLoaded(scn_guid)) {
			m_active_scene_guid = scn_guid;
		}
	}
	inline rp::Guid GetActiveSceneGuid() const {
		return m_active_scene_guid;
	}
	inline std::optional<std::reference_wrapper<Scene>> GetActiveScene() {
		if (m_active_scene_guid == rp::null_guid) {
			// Return first loaded scene if no active scene set
			if (!m_loaded_scenes.empty()) {
				return std::make_optional(std::ref(m_loaded_scenes.begin()->second));
			}
			return std::nullopt;
		}
		return GetScene(m_active_scene_guid);
	}

	inline void SetSceneWorkingDir(const std::string& path) {
		m_SceneWorkingDir = path;
	}
	inline std::string_view GetSceneWorkingDir() const {
		return m_SceneWorkingDir;
	}
	void onCreateAssignToDefault(ecs::entity e);
	void onCreateAssignSceneIDToDefault(ecs::entity e);
	void onDestroySceneComponent(ecs::entity e);
	//nullopt if not found
	inline std::optional<ecs::entity> GetReferencedEntity(SceneEntityReference const& ref) {
		auto scnres = GetScene(ref.m_scene_guid.m_guid);
		auto entityres = scnres ? scnres.value().get().GetSceneEntity(ref.m_scene_id) : std::nullopt;
		return entityres;
	}
	void Clear() {
		for (auto& [sid, scn] : m_loaded_scenes) {
			scn.Clear();
		}
		m_loaded_scenes.clear();
	}
};

RegisterReflectionTypeBegin(SceneIDComponent, "Scene ID")
	MemberRegistrationV<&SceneIDComponent::m_scene_id, "Scene ID">
RegisterReflectionTypeEnd

/*
RegisterReflectionTypeBegin(SceneComponent, "Scene ID")
	MemberRegistrationV<&SceneComponent::m_scene_guid, "Scene Guid">
RegisterReflectionTypeEnd
*/

//dsd 
#endif