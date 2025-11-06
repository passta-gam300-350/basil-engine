#ifndef MonoEntityManager_HPP
#define MonoEntityManager_HPP
#include <functional>

#include "Manager/MonoTypeRegistry.hpp"
#include <unordered_map>
#include <memory>

#include "rsc-core/rp.hpp"

struct CSKlass;
struct CSKlassInstance;
struct ManagedAssembly;

class MonoEntityManager {

	using ClassVisitor = std::function<void(CSKlass&)>;

	using ScriptID = rp::Guid;
	ScriptID PRIMARY_ASSEMBLY_ID{ 0x0, 0x0 };
	ScriptID BACKEND_ASSEMBLY_ID{ 0x0,0x0 };

	std::string searchScriptIn;


	std::vector<std::unique_ptr<ManagedAssembly>> m_Assemblies;
	std::vector<std::shared_ptr<CSKlass>> m_Klasses;
	std::vector<std::unique_ptr<CSKlassInstance>> m_Instances;


	std::unordered_map<ScriptID, size_t> m_AssemblyMap;
	std::unordered_map<ScriptID, size_t> m_EntityInstanceMap;
	std::unordered_map<ScriptID, size_t> m_EntityMap;

	std::unordered_map<std::string, ScriptID> m_NamedKlassMap;

	MonoTypeRegistry m_TypeRegistry;

	bool preCompiled = false;
	bool useDefault = true;

public:
	

	MonoEntityManager();
	static MonoEntityManager& GetInstance();

	void initialize();
	void StartCompilation();


	void AddSearchDirectory(const char* path);
	void SetOutputDirectory(const char* path);

	ScriptID AddAssembly(std::unique_ptr<ManagedAssembly> assembly);
	ScriptID AddAssembly(const char* assemblyPath, bool isSystem);

	ScriptID AddKlass(std::shared_ptr<CSKlass> klass);
	ScriptID AddKlass(const char* klassName, const char* klassNamespace = "", bool isBackend=false);

	void AddNamedKlass(const char* klassName, const char* klassNamespace = "", bool isBackend = false);
	void AddNamedKlass(std::shared_ptr<CSKlass> klass);

	ScriptID InstanceFrom(CSKlass const& klass);
	ScriptID AddInstance(std::unique_ptr<CSKlassInstance> instance);
	ScriptID AddInstance(const char* klassName, const char* klassNamespace = "", void* args[]=nullptr, bool isBackend = false);


	ScriptID GetPrimaryAssembly();
	ScriptID GetEngineAssembly();

	ManagedAssembly* GetAssembly(ScriptID id);
	CSKlass* GetKlass(ScriptID id);	
	CSKlassInstance* GetInstance(ScriptID id);


	CSKlass* GetNamedKlass(const char* klassName, const char* klassNamespace);

	void VisitAssembly(ManagedAssembly* assembly, const ClassVisitor& visitor);
	void AddKlassFromAssembly(ScriptID assemblyID);

	void SetPreCompiled(bool val) {
		preCompiled = val;
	}

	MonoTypeRegistry& GetTypeRegistry() {
		return m_TypeRegistry;
	}


	void ClearAll();
	~MonoEntityManager();

	void Attach();
	void Detach();

};

#endif // MonoEntityManager_HPP