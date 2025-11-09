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


