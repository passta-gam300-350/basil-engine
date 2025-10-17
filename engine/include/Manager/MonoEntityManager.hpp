#ifndef MonoEntityManager_HPP
#define MonoEntityManager_HPP
#include "serialisation/guid.h"
#include <unordered_map>
#include <memory>

struct CSKlass;
struct CSKlassInstance;

class MonoEntityManager {
	using ScriptID = Resource::Guid;
	std::vector<std::shared_ptr<CSKlass>> m_Klasses;
	std::vector<std::unique_ptr<CSKlassInstance>> m_Instances;
	std::unordered_map<ScriptID, size_t> m_EntityInstanceMap;
	std::unordered_map<size_t, ScriptID> m_EntityMap;

public:
	MonoEntityManager();
	static MonoEntityManager& GetInstance();

	ScriptID AddKlass(std::shared_ptr<CSKlass> klass);
	ScriptID AddInstance(std::unique_ptr<CSKlassInstance> instance);

	CSKlass* GetKlass(ScriptID id);	
	CSKlassInstance* GetInstance(ScriptID id);

	~MonoEntityManager();


};

#endif // MonoEntityManager_HPP