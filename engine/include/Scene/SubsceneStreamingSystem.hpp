#ifndef ENG_SUBSCENE_STREAMING_SYSTEM_HPP
#define ENG_SUBSCENE_STREAMING_SYSTEM_HPP

#include "Scene/Scene.hpp"
#include "Ecs/ecs.h"
#include "Render/Camera.h"
#include <glm/gtx/norm.hpp>

struct SubsceneStreamingSystem : public ecs::SystemBase
{
	void Init() override {}
	void Update(ecs::world&, float) override {
	
	}
	void FixedUpdate(ecs::world& w) override {
		auto meta_sss{ w.filter_entities<SubsceneMetaComponent>() };
		auto cam_pos{CameraSystem::GetActiveCamera().m_Pos};

		for (auto ss_meta : meta_sss) {
			auto& comp = ss_meta.get<SubsceneMetaComponent>();
			auto res = SceneRegistry::GetScene(comp.impl.scene_id);
			if (res) {
				ss_meta.destroy();
				continue;
			}
			auto& scn = res.value().get();
			auto ssres = scn.GetSubscene(comp.impl.subscene_id);
			if (comp.m_settings.Pesistent || !comp.m_settings.ProximityStreaming) {
				continue;
			}
			if (glm::all(glm::greaterThanEqual(cam_pos, comp.m_ss_bv_min)) && glm::all(glm::lessThanEqual(cam_pos, comp.m_ss_bv_max))) {
				if (comp.m_settings.LoadState == SubsceneState::Unloaded && !ssres) {
					//load				
				}
			}
			glm::vec3 center{ (comp.m_ss_bv_min + comp.m_ss_bv_max) / 2.f };
			glm::vec3 hfbounds{ (comp.m_ss_bv_max - comp.m_ss_bv_min) / 2.f };
			glm::vec3 dist{ cam_pos - center };
			//inaccurate cheat //lazy do the proper math
			if (glm::length2(dist) < glm::length2(hfbounds + glm::vec3(scn.m_proximity_load_radius))) {
				//load
			}
			else if(ssres){
				scn.Unload();
			}
		}
	}
	void Exit() override {
	
	}
	~SubsceneStreamingSystem() {};
};

#endif