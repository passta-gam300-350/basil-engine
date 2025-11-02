#include "Manager/MonoEntityManager.hpp"
#include "ABI/ABI.h"
#include "MonoManager.hpp"
#include "MonoLoader.hpp"
#include "ScriptCompiler.hpp"
#include "Manager/MonoTypeResolver.hpp"
#include "Manager/MonoReflectionRegistry.hpp"
#include "Reflection/MonoReflectionBackend.hpp"
#include "MonoResolver/MonoTypeDescriptor.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <mono/metadata/metadata.h>
#include <mono/metadata/attrdefs.h>


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
	if (isSystem)
		MonoManager::GetLoader()->Enable_BackEnd();
	else MonoManager::GetLoader()->Enable_Game();
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

	if (false)
		MonoManager::GetLoader()->Enable_BackEnd();
	else MonoManager::GetLoader()->Enable_Game();

	auto klass = MonoManager::GetKlass(MonoEntityManager::GetInstance().GetAssembly((isBackend) ? BACKEND_ASSEMBLY_ID : PRIMARY_ASSEMBLY_ID), klassName, klassNamespace);

	return AddKlass(std::move(klass));
}

Resource::Guid MonoEntityManager::AddInstance(const char* klassName, const char* klassNamespace, void* args[], bool isBackend) {
	// Check if klass exists
	auto klass = GetNamedKlass(klassName, klassNamespace);
	if (klass) {
		auto instance = MonoManager::CreateInstance(MonoManager::GetLoader()->GetActiveDomain(), *klass, args);
		return AddInstance(std::move(instance));

	}
	// Create klass if not exists

	AddNamedKlass(klassName, klassNamespace, isBackend);
	auto klassPtr = GetNamedKlass(klassName, klassNamespace);
	auto instance = MonoManager::CreateInstance(MonoManager::GetLoader()->GetActiveDomain(), *klassPtr, args);
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
	std::string fullName;
	if (std::string(klassNamespace).empty()) {
		fullName = std::string(klassName);
	}
	else {
		fullName = std::string(klassNamespace) + "." + std::string(klassName);
	}

	m_TypeRegistry.Register(fullName, klassID);

	MonoTypeDescriptor descriptor{};
	descriptor.kind = Kind::Custom;
	descriptor.managedKind = ManagedKind::Custom;
	descriptor.managed_name = fullName;
	descriptor.cpp_name = fullName;
	descriptor.isEngineType = isBackend;
	descriptor.isUserType = !isBackend;
	descriptor.isSerializable = true;
	descriptor.isPublic = true;
	descriptor.guid = klassID;

	const MonoTypeDescriptor* typeDescriptor = MonoTypeResolver::Instance().RegisterDescriptor(std::move(descriptor));

	ManagedAssembly* assembly = GetAssembly(isBackend ? BACKEND_ASSEMBLY_ID : PRIMARY_ASSEMBLY_ID);
	if (assembly)
	{
		const std::string assemblyName(assembly->Name());
		const std::string fullNameCopy = fullName;
		MonoReflectionRegistry& registry = MonoReflectionRegistry::Instance();
		ClassNode* classNode = registry.GetOrAddClass(assemblyName, fullNameCopy);
		if (classNode)
		{
			if (!classNode->descriptor)
			{
				classNode->descriptor = typeDescriptor;
			}

			if (CSKlass* klassPtr = GetKlass(klassID))
			{
				MonoReflectionBackend::EnumerateField(klassPtr, [assemblyName, fullNameCopy, &registry](const char* fieldName, const char* typeName, CSKlass* klass, MonoClassField* field)
					{
						(void)klass;
						const bool isStatic = (mono_field_get_flags(field) & MONO_FIELD_ATTR_STATIC) != 0;
						const uint32_t accessMask = mono_field_get_flags(field) & MONO_FIELD_ATTR_FIELD_ACCESS_MASK;
						const bool isPublic = accessMask == MONO_FIELD_ATTR_PUBLIC;
						registry.RegisterField(assemblyName, fullNameCopy, fieldName ? fieldName : "", typeName ? typeName : "", isStatic, isPublic);
					});
			}
		}
	}

}


void MonoEntityManager::AddNamedKlass(std::shared_ptr<CSKlass> klass) {
	auto klassID = AddKlass(klass);
	std::string fullName;
	if (std::string(klass->Namespace()).empty()) {
		fullName = std::string(klass->Name());
	}
	else {
		fullName = std::string(klass->Namespace()) + "." + std::string(klass->Name());
	}
	m_TypeRegistry.Register(fullName, klassID);

	MonoTypeDescriptor descriptor{};
	descriptor.kind = Kind::Custom;
	descriptor.managedKind = ManagedKind::Custom;
	descriptor.managed_name = fullName;
	descriptor.cpp_name = fullName;
	descriptor.isEngineType = false;
	descriptor.isUserType = true;
	descriptor.isSerializable = true;
	descriptor.isPublic = true;
	descriptor.guid = klassID;

	const MonoTypeDescriptor* typeDescriptor = MonoTypeResolver::Instance().RegisterDescriptor(std::move(descriptor));

	ManagedAssembly* owningAssembly = nullptr;
	if (klass)
	{
		MonoImage* image = klass->Image();
		if (image)
		{
			for (auto& assemblyPtr : m_Assemblies)
			{
				if (assemblyPtr && assemblyPtr->Image() == image)
				{
					owningAssembly = assemblyPtr.get();
					break;
				}
			}
		}
	}

	if (owningAssembly && klass)
	{
		const std::string assemblyName(owningAssembly->Name());
		const std::string fullNameCopy = fullName;

		MonoReflectionRegistry& registry = MonoReflectionRegistry::Instance();
		ClassNode* classNode = registry.GetOrAddClass(assemblyName, fullNameCopy);
		if (classNode && !classNode->descriptor)
		{
			classNode->descriptor = typeDescriptor;
		}

	MonoReflectionBackend::EnumerateField(klass.get(), [assemblyName, fullNameCopy, &registry](const char* fieldName, const char* typeName, CSKlass* csKlass, MonoClassField* field)
		{
			(void)csKlass;
			const bool isStatic = (mono_field_get_flags(field) & MONO_FIELD_ATTR_STATIC) != 0;
			const uint32_t accessMask = mono_field_get_flags(field) & MONO_FIELD_ATTR_FIELD_ACCESS_MASK;
			const bool isPublic = accessMask == MONO_FIELD_ATTR_PUBLIC;
			registry.RegisterField(assemblyName, fullNameCopy, fieldName ? fieldName : "", typeName ? typeName : "", isStatic, isPublic);
		});
}
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
	std::string fullName;
	if (std::string(klassNamespace).empty()) {
		fullName = std::string(klassName);
	}
	else {
		fullName = std::string(klassNamespace) + "." + std::string(klassName);
	}

	auto const id = m_TypeRegistry.GetMonoEntityID(fullName);
	return GetKlass(id);
}

void MonoEntityManager::AddKlassFromAssembly(const ScriptID assemblyID) {
	auto assembly = GetAssembly(assemblyID);
	if (!assembly || !assembly->IsLoaded()) {
		return;
	}
	auto klasses = MonoManager::LoadKlassesFromAssembly(assembly);
	for (auto& klass : klasses) {
		AddNamedKlass(klass); // Register a NAMED class instead of an anonymous class
	}
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
	MonoReflectionRegistry::Instance().Clear();
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
	MonoTypeResolver::Instance();
	MonoReflectionRegistry::Instance().Clear();



}

void MonoEntityManager::StartCompilation() {
	if (useDefault) {
		for (const char* bucket : scriptBuckets) {
			MonoManager::AddSearchDirectories(bucket);
		}
		MonoManager::GetCompiler()->SetCompileOutputDirectory(asmOutputDir);
	}


	std::filesystem::path asmPath = std::filesystem::absolute(MonoManager::GetCompiler()->GetCompileOutputDirectory()) / (MonoManager::GetCompiler()->GetCompileOutputName() + ".dll");

	//TODO: Move it to cmake build
	std::filesystem::path backendPath = R"(C:\Users\yeo_j\Documents\Digipen Repo\Year 3\Project\Project\engine\managed\BasilEngine\bin\Release\net48\BasilEngine.dll)";

	if (preCompiled) {
		std::cout << "Using pre-compiled assemblies." << std::endl;
	}
	else {
		MonoManager::StartCompilation();


	}
	auto& logs = MonoManager::GetCompiler()->GetLogs();

	if (logs.empty()) {
		PRIMARY_ASSEMBLY_ID = AddAssembly(asmPath.string().c_str());
		BACKEND_ASSEMBLY_ID = AddAssembly(backendPath.string().c_str());

		// Load all klasses from assemblies - Backend first
		AddKlassFromAssembly(BACKEND_ASSEMBLY_ID);
	}
	else {
		PRIMARY_ASSEMBLY_ID = AddAssembly(asmPath.string().c_str());
		BACKEND_ASSEMBLY_ID = AddAssembly(backendPath.string().c_str());

		// Load all klasses from assemblies - Backend first
		AddKlassFromAssembly(BACKEND_ASSEMBLY_ID);
		std::cout << "Info generated, see logs above." << std::endl;
	}
}

void MonoEntityManager::AddSearchDirectory(const char* path) {
	std::filesystem::path file_path = path;
	std::cout << std::filesystem::absolute(file_path).string() << std::endl;
	if (std::filesystem::is_directory(file_path)) {
		MonoManager::GetCompiler()->AddSearchDirectories("DEFAULT", std::filesystem::absolute(file_path).string());
	}
	useDefault = false;
}

void MonoEntityManager::SetOutputDirectory(const char* path) {
	std::filesystem::path file_path = path;
	MonoManager::GetCompiler()->SetCompileOutputDirectory(std::filesystem::absolute(file_path).string());
	useDefault = false;
}

void MonoEntityManager::Attach() {
	MonoManager::Attach();
}

void MonoEntityManager::Detach() {
	MonoManager::Detach();
}
