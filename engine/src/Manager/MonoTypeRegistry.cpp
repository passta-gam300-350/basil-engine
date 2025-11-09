/******************************************************************************/
/*!
\file   MonoTypeRegistry.cpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the implementation for the MonoTypeRegistry class, which
is responsible for registering and retrieving Mono type information
based on type names and script IDs.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Manager/MonoTypeRegistry.hpp"
#include "Manager/StringManager.hpp"

void MonoTypeRegistry::Register(std::string typeName, ScriptID id) {
	const uint32_t hash = StringManager::GetInstance().Intern_String(std::move(typeName));
	m_TypeMap[hash] = id;


}

void MonoTypeRegistry::Unregister(std::string typeName) {
	const uint32_t hash = StringManager::GetInstance().Intern_String(std::move(typeName));
	m_TypeMap.erase(hash);
}

MonoTypeRegistry::ScriptID MonoTypeRegistry::GetMonoEntityID(std::string name) const {
	const uint32_t hash = StringManager::GetInstance().Intern_String(std::move(name));
	auto it = m_TypeMap.find(hash);
	if (it != m_TypeMap.end()) {
		return it->second;
	}
	return ScriptID{ 0x0, 0x0 };
	
}


