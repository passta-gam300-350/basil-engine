#ifndef ENG_SCENE_HPP
#define ENG_SCENE_HPP

#include "ecs/internal/world.h"
#include "serialisation/guid.h"
#include <glm/glm.hpp>

struct SceneIDComponent {
	union {
		struct detail {
			std::uint64_t scene_id : 16;
			std::uint64_t subscene_id : 16;
			std::uint64_t id : 32;
		} impl;
		std::uint64_t handle;
	};
};

enum class SubsceneState : uint8_t
{
	Unloaded,
	Loading,
	Loaded,
	Active,
	Unloading
};

struct SubsceneMetaComponent {
	struct detail {
		std::uint32_t scene_id : 16;
		std::uint32_t subscene_id : 16;
	} impl;
	glm::vec3 m_ss_bv_min;
	glm::vec3 m_ss_bv_max;
	struct SubsceneSettings {
		SubsceneState LoadState;
		bool Pesistent{ false };
		bool Startup{ false }; //flag for on load scene 
		bool ProximityStreaming{ true }; //load via proximity (disable for manual loading)
	}m_settings;
};

struct Subscene {
	std::uint32_t m_ssid;
	std::string m_name;
	std::unordered_map<std::uint32_t, ecs::entity> m_ss_entities;
	std::vector<rp::Guid> m_subscene_dependencies;

	inline void Clear() {
		for (auto [seid, e] : m_ss_entities) {
			e.destroy();
		}
	}
	void Unload();
};

//this is the native form of scene, we serialises this
struct Scene
{
	Scene() = default;
	Scene(std::string const& name);
	inline void Clear() {
		for (auto& [ssid, ss] : m_loaded_ss) {
			ss.Clear();
		}
	}
	void Unload();

	//gets the loaded subscene
	inline std::optional<std::reference_wrapper<Subscene>> GetSubscene(std::uint32_t ssid) {
		auto it = m_loaded_ss.find(ssid);
		return it != m_loaded_ss.end() ? std::make_optional(std::ref(it->second)) : std::nullopt;
	}

//private:
	std::uint32_t m_scene_id;
	std::string m_name;
	std::unordered_map<std::uint32_t, Subscene> m_loaded_ss;
	std::vector<rp::Guid> m_scene_dependencies;
	float m_proximity_load_radius;
};

struct SceneRegistry{
private:
	std::unordered_map<std::uint32_t, rp::Guid> m_scenelist;
	std::unordered_map<std::uint32_t, Scene> m_loaded_scenes;
	
	inline static std::unique_ptr<SceneRegistry>& InstancePtr() {
		static std::unique_ptr<SceneRegistry> s_scn_reg{std::make_unique<SceneRegistry>()};
		return s_scn_reg;
	}

public:
	SceneRegistry();
	~SceneRegistry() {
		for (auto& [sid, scn] : m_loaded_scenes) {
			scn.Clear();
		}
	}

	inline static SceneRegistry& Instance() {
		return *InstancePtr();
	}
	inline static void Release() {
		InstancePtr().reset(nullptr);
	}
	inline static void AddSceneGuid(std::uint32_t sid, rp::Guid sguid) {
		Instance().m_scenelist.try_emplace(sid, sguid);
	}
	inline static void EraseSceneGuid(std::uint32_t sid) {
		Instance().m_loaded_scenes[sid].Clear();
		Instance().m_loaded_scenes.erase(sid);
		Instance().m_scenelist.erase(sid);
	}
	static Scene& LoadScene(std::uint32_t sid);
	inline static Scene& AddLoadedScene(std::uint32_t sid, Scene&& scn) {
		Instance().m_loaded_scenes.try_emplace(sid, std::forward<Scene>(scn));
	}
	inline static void UnloadScene(std::uint32_t sid) {
		auto& loaded_reg{ Instance().m_loaded_scenes };
		if (auto it{ loaded_reg.find(sid) }; it != loaded_reg.end()) {
			it->second.Clear();
			loaded_reg.erase(it);
		}
	}
	inline static std::optional<std::reference_wrapper<Scene>> GetScene(std::uint32_t sid) {
		auto it{ Instance().m_loaded_scenes.find(sid) };
		return it != Instance().m_loaded_scenes.end() ? std::make_optional(std::ref(it->second)) : std::nullopt;
	}
};

#endif