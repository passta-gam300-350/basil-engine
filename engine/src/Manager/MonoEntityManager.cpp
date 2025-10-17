#include "Manager/MonoEntityManager.hpp"
#include "ABI/ABI.h"
#include "MonoManager.hpp"
#include "MonoLoader.hpp"
#include "ScriptCompiler.hpp"

#include <filesystem>
#include <iostream>


MonoEntityManager::MonoEntityManager() {
	m_Klasses.reserve(100);
	m_Instances.reserve(100);
}

MonoEntityManager::~MonoEntityManager() = default;

MonoEntityManager& MonoEntityManager::GetInstance() {
	static MonoEntityManager instance;
	return instance;
}


Resource::Guid MonoEntityManager::AddAssembly(std::unique_ptr<ManagedAssembly> assembly) {
	const Resource::Guid id = Resource::Guid::generate();
	m_AssemblyMap[id] = m_Assemblies.size();
	m_Assemblies.push_back(std::move(assembly));
	return id;
}

Resource::Guid MonoEntityManager::AddAssembly(const char* assemblyPath, bool isSystem=false) {
	auto assembly = MonoManager::GetLoader()->LoadAssembly(assemblyPath, ((isSystem) ? MonoManager::GetLoader()->GetBackendDomain() : MonoManager::GetLoader()->GetGameDomain()));
	return AddAssembly(std::move(assembly));
}

Resource::Guid MonoEntityManager::AddKlass(std::shared_ptr<CSKlass> klass) {
	
	const Resource::Guid id = Resource::Guid::generate();
	m_EntityMap[id] = m_Klasses.size();
	m_Klasses.push_back(std::move(klass));
	return id;
}

Resource::Guid MonoEntityManager::AddInstance(std::unique_ptr<CSKlassInstance> instance) {
	const Resource::Guid id = Resource::Guid::generate();
	m_EntityInstanceMap[id] = m_Instances.size();
	m_Instances.push_back(std::move(instance));
	return id;
}


CSKlass* MonoEntityManager::GetKlass(const ScriptID id) {
	auto it = m_EntityMap.find(id);
	if (it != m_EntityMap.end()) {
		return m_Klasses.at(it->second).get();
	}
	return nullptr;
}

CSKlassInstance* MonoEntityManager::GetInstance(const ScriptID id) {
	auto it = m_EntityInstanceMap.find(id);
	if (it != m_EntityInstanceMap.end()) {
		return m_Instances.at(it->second).get();
	}
	return nullptr;
}


const char* scriptBuckets[] = {
	"Scripts/Entities",
	"../scripts"
};

const char* asmOutputDir = "ScriptBins";
void MonoEntityManager::initialize() {
	m_Assemblies.clear();
	m_Klasses.clear();
	m_Instances.clear();
	m_AssemblyMap.clear();
	m_EntityInstanceMap.clear();
	m_EntityMap.clear();
	
	MonoManager::Initialize();
	for (const char* bucket : scriptBuckets) {
		MonoManager::AddSearchDirectories(bucket);
	}
	MonoManager::GetCompiler()->SetCompileOutputDirectory(asmOutputDir);



	std::filesystem::path asmPath = std::filesystem::absolute(asmOutputDir) / MonoManager::GetCompiler()->GetCompileOutputName() / ".dll";
	MonoManager::StartCompilation();
	

	auto& logs = MonoManager::GetCompiler()->GetLogs();

	if (logs.empty()) {
		PRIMARY_ASSEMBLY_ID = AddAssembly(asmPath.string().c_str());
	}
	else {
		PRIMARY_ASSEMBLY_ID = AddAssembly(asmPath.string().c_str());
		std::cout << "Info generated, see logs above." << std::endl;
	}
	
}





