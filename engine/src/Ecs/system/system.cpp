/******************************************************************************/
/*!
\file   system.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  ECS system implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include <Ecs/ecs.h>

namespace {
	constexpr std::uint64_t DEFAULT_SYSTEM_THREADS{ 4 };
	constexpr bool DEFAULT_SYSTEM_STATE{ true };
}

namespace ecs {
	void SystemRegistry::LoadConfig(YAML::Node& cfg) {
		if (cfg["system threads"]){
			Scheduler::SetSystemThreads(cfg["system threads"].as<std::int32_t>());
		}
		if (cfg["enabled system"]) {
			for (auto& s : Instance().m_AllSystems) {
				s.m_Enabled = true;
				Instance().m_ActiveSystems.emplace(s.m_Id, s.m_Factory());
				Instance().m_ActiveSystems[s.m_Id]->Init();
			}
		}
		if (cfg["system configurations"]) {

		}
	}

	YAML::Node SystemRegistry::GetDefaultConfig() {
		YAML::Node root{};
		YAML::Node enabled{};
		YAML::Node configs{};
		root["system threads"] = DEFAULT_SYSTEM_THREADS;
		for (auto system : SystemRegistry::Instance().m_AllSystems) {
			enabled[system.m_Name] = DEFAULT_SYSTEM_STATE;
		}
		root["enabled system"] = enabled;
		root["system configurations"] = configs;
		return root;
	}

	void SystemRegistry::Exit() {
		for (auto& [id, s] : Instance().m_ActiveSystems) {
			s->Exit();
		}
	}
}