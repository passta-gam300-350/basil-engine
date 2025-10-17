#ifndef MonoEntityManager_HPP
#define MonoEntityManager_HPP
#include "serialisation/guid.h"
#include <unordered_map>
#include <memory>

struct CSKlass;
struct CSKlassInstance;
struct ManagedAssembly;

class MonoEntityManager {
	using ScriptID = Resource::Guid;
	ScriptID PRIMARY_ASSEMBLY_ID{ 0x0, 0x0 };
	std::vector<std::unique_ptr<ManagedAssembly>> m_Assemblies;
	std::vector<std::shared_ptr<CSKlass>> m_Klasses;
	std::vector<std::unique_ptr<CSKlassInstance>> m_Instances;


	std::unordered_map<ScriptID, size_t> m_AssemblyMap;
	std::unordered_map<ScriptID, size_t> m_EntityInstanceMap;
	std::unordered_map<ScriptID, size_t> m_EntityMap;

public:
	MonoEntityManager();
	static MonoEntityManager& GetInstance();

	void initialize();


	ScriptID AddAssembly(std::unique_ptr<ManagedAssembly> assembly);
	ScriptID AddAssembly(const char* assemblyPath, bool isSystem);

	ScriptID AddKlass(std::shared_ptr<CSKlass> klass);
	ScriptID AddKlass(const char* klassName, const char* klassNamespace = "");
	ScriptID AddInstance(std::unique_ptr<CSKlassInstance> instance);
	ScriptID AddInstance(const char* klassName, const char* klassNamespace = "");
	

	ManagedAssembly* GetAssembly(ScriptID id);
	CSKlass* GetKlass(ScriptID id);	
	CSKlassInstance* GetInstance(ScriptID id);


	~MonoEntityManager();


};

#endif // MonoEntityManager_HPP