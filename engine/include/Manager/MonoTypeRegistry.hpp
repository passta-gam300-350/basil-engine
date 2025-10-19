#ifndef MONO_TYPE_REGISTRY_HPP
#define MONO_TYPE_REGISTRY_HPP
#include "serialisation/guid.h"
#include <unordered_map>
class MonoTypeRegistry {
	using ScriptID = Resource::Guid;
	std::unordered_map<uint32_t, ScriptID> m_TypeMap;

public:
	void Register(std::string typeName, ScriptID id);
	void Unregister(std::string typeName);
	ScriptID GetMonoEntityID(std::string name) const;
};


#endif