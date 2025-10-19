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

Resource::Guid MonoEntityManager::AddAssembly(const char* assemblyPath, bool isSystem = false) {
	auto assembly = MonoManager::GetLoader()->LoadAssembly(assemblyPath, ((isSystem) ? MonoManager::GetLoader()->GetBackendDomain() : MonoManager::GetLoader()->GetGameDomain()));
	return AddAssembly(std::move(assembly));
}

Resource::Guid MonoEntityManager::AddKlass(std::shared_ptr<CSKlass> klass) {

	const Resource::Guid id = Resource::Guid::generate();
	m_EntityMap[id] = m_Klasses.size();
	m_Klasses.push_back(std::move(klass));
	return id;
}

Resource::Guid MonoEntityManager::AddKlass(const char* klassName, const char* klassNamespace, bool isBackend) {
	auto klass = MonoManager::GetKlass(MonoEntityManager::GetInstance().GetAssembly((isBackend) ? BACKEND_ASSEMBLY_ID : PRIMARY_ASSEMBLY_ID), klassName, klassNamespace);
	return AddKlass(std::move(klass));
}

Resource::Guid MonoEntityManager::AddInstance(const char* klassName, const char* klassNamespace, bool isBackend) {
	// Check if klass exists
	auto klass = GetNamedKlass(klassName, klassNamespace);
	if (klass) {
		auto instance = MonoManager::CreateInstance(MonoManager::GetLoader()->GetActiveDomain(), *klass);
		return AddInstance(std::move(instance));

	}
	// Create klass if not exists
	auto klassID = AddKlass(klassName, klassNamespace, isBackend);
	auto klassPtr = GetKlass(klassID);
	auto instance = MonoManager::CreateInstance(MonoManager::GetLoader()->GetActiveDomain(), *klassPtr);
	return AddInstance(std::move(instance));
}


Resource::Guid MonoEntityManager::AddInstance(std::unique_ptr<CSKlassInstance> instance) {
	const Resource::Guid id = Resource::Guid::generate();
	m_EntityInstanceMap[id] = m_Instances.size();
	m_Instances.push_back(std::move(instance));
	return id;
}

void MonoEntityManager::AddNamedKlass(const char* klassName, const char* klassNamespace, bool isBackend) {
	auto klassID = AddKlass(klassName, klassNamespace, isBackend);
	std::string fullName = std::string(klassNamespace) + "." + std::string(klassName);

	m_TypeRegistry.Register(fullName, klassID);

}


ManagedAssembly* MonoEntityManager::GetAssembly(const ScriptID id) {
	auto it = m_AssemblyMap.find(id);
	if (it != m_AssemblyMap.end()) {
		return m_Assemblies.at(it->second).get();
	}
	return nullptr;
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



CSKlass* MonoEntityManager::GetNamedKlass(const char* klassName, const char* klassNamespace) {
	std::string fullName = std::string(klassNamespace) + "." + std::string(klassName);
	auto const id = m_TypeRegistry.GetMonoEntityID(fullName);
	return GetKlass(id);
}


// Invalidate all script id and clear all maps and vectors
void MonoEntityManager::ClearAll() {
	m_Assemblies.clear();
	m_Klasses.clear();
	m_Instances.clear();
	m_AssemblyMap.clear();
	m_EntityInstanceMap.clear();
	m_EntityMap.clear();
	m_NamedKlassMap.clear();
	m_TypeRegistry = MonoTypeRegistry{};
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



	std::filesystem::path asmPath = std::filesystem::absolute(asmOutputDir) / (MonoManager::GetCompiler()->GetCompileOutputName() + ".dll");
	//TODO: Move it to cmake build
	std::filesystem::path backendPath = R"(C:\Users\yeo_j\Documents\Digipen Repo\Year 3\Project\Project\engine\managed\BasilEngine\bin\BasilEngine.dll)";

	MonoManager::StartCompilation();


	auto& logs = MonoManager::GetCompiler()->GetLogs();

	if (logs.empty()) {
		PRIMARY_ASSEMBLY_ID = AddAssembly(asmPath.string().c_str());
		BACKEND_ASSEMBLY_ID = AddAssembly(backendPath.string().c_str(), true);
	}
	else {
		PRIMARY_ASSEMBLY_ID = AddAssembly(asmPath.string().c_str());
		BACKEND_ASSEMBLY_ID = AddAssembly(backendPath.string().c_str(), true);
		std::cout << "Info generated, see logs above." << std::endl;
	}

}





