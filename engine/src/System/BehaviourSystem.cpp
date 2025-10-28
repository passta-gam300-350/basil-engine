#include "MonoManager.hpp"
#include "System/BehaviourSystem.hpp"
#include "Manager/MonoEntityManager.hpp"
#include "components/behaviour.hpp"
#include "Engine.hpp"
#include "ABI/ABI.h"
#include "spdlog/spdlog.h"
const char* defSearchDirs[] = {
	"Scripts/Bin/",
	"Scripts/Bin/Debug/",
	"Scripts/Bin/Release/"
};

const char* defAssembly = "Scripts/Bin/GameAssembly.dll";


void parse_class_name(const std::string& fullName, std::string& outNamespace, std::string& outClassName) {
	// Find the last dot, which separates namespace and class name
	size_t pos = fullName.find_last_of('.');
	if (pos != std::string::npos) {
		outNamespace = fullName.substr(0, pos);
		outClassName = fullName.substr(pos + 1);
	}
	else {
		// No namespace found
		outNamespace.clear();
		outClassName = fullName;
	}
}

BehaviourSystem& BehaviourSystem::Instance() {
	static BehaviourSystem instance;
	return instance;
}

void BehaviourSystem::Init()
{
	MonoEntityManager::GetInstance().Attach(); // A

	auto world = Engine::GetWorld();

	auto entities = world.filter_entities<behaviour>();
	for (auto entity : entities) {
		behaviour& component = world.get_component_from_entity<behaviour>(entity);
		for (auto val : component.classesName) {
			std::string className, namespaceName;
			parse_class_name(val, namespaceName, className);

			if (namespaceName.empty()) {
				AddScriptToEntityComponent(entity, world, className.c_str());
			}
			else AddScriptToEntityComponent(entity, world, className.c_str(), namespaceName.c_str());
		}
	}


}
void BehaviourSystem::Update(ecs::world& world, float dt)
{
	for (auto entity : world.filter_entities<behaviour>()) {
		behaviour& component = world.get_component_from_entity<behaviour>(entity);
		for (auto scriptID : component.scriptIDs) {
			CSKlassInstance* instance = MonoEntityManager::GetInstance().GetInstance(scriptID);
			if (instance) {
				instance->Invoke("Update", nullptr, nullptr,0);
			}
		}
	}
}
void BehaviourSystem::FixedUpdate(ecs::world& world)
{
	for (auto entity : world.filter_entities<behaviour>()) {
		behaviour& component = world.get_component_from_entity<behaviour>(entity);
		for (auto scriptID : component.scriptIDs) {
			CSKlassInstance* instance = MonoEntityManager::GetInstance().GetInstance(scriptID);
			if (instance) {
				instance->Invoke("FixedUpdate", nullptr, nullptr, 0);
			}
		}
	}
}
void BehaviourSystem::Exit()
{
}

void BehaviourSystem::AddScriptToEntityComponent(ecs::entity& entity, ecs::world& world, Resource::Guid scriptID) {
	behaviour& component = world.get_component_from_entity<behaviour>(entity);

	

	CSKlassInstance* instance = MonoEntityManager::GetInstance().GetInstance(scriptID);
	
	if (!instance || !instance->IsValid()) {
		spdlog::warn("[BehaviourSystem] Failed to create instance for script ID: {}", scriptID.to_hex());
		return;
	}
	const CSKlass* klass = instance->Klass();
	auto derivedClass = MonoEntityManager::GetInstance().GetNamedKlass("Behavior", "BasilEngine.Components");
	if (!derivedClass) {
		MonoEntityManager::GetInstance().AddNamedKlass("Behavior", "BasilEngine.Components", true);
		derivedClass = MonoEntityManager::GetInstance().GetNamedKlass("Behavior", "BasilEngine.Components");
		if (!derivedClass) {
			spdlog::warn("[BehaviourSystem] BasilEngine.Components.Behaviour class not found in assembly!");
			return;
		}
	}
	bool derived = klass->IsDerivedFrom(*derivedClass);
	if (!derived) {
		spdlog::warn("[BehaviorSystem] Script ID: {} is not derived from BasilEngine.Behavior", scriptID.to_hex());
		return;
	}

	
	component.scriptIDs.push_back(scriptID);

}

void BehaviourSystem::AddScriptToEntityComponent(ecs::entity& entity, ecs::world& world, const char* klassname, const char* klass_ns) 
{
	CSKlass* klass = MonoEntityManager::GetInstance().GetNamedKlass(klassname, klass_ns);
	// Check if klass is derived from Behaviour
	if (!klass) {
		spdlog::info("Class Not found - Try to create it");
		MonoEntityManager::GetInstance().AddNamedKlass(klassname, klass_ns);
		klass = MonoEntityManager::GetInstance().GetNamedKlass(klassname, klass_ns);
		if (!klass) {
			spdlog::warn("Class not found in assembly!");
			return;
		}
	}
	bool derived = klass->IsDerivedFrom("BasilEngine.Components.Behaviour");
	if (!derived) {
		spdlog::warn("[BehaviourSystem] Class {}.{} is not derived from BasilEngine.Behaviour", klass_ns, klassname);
		return;
	}
	Resource::Guid scriptID = MonoEntityManager::GetInstance().AddInstance(klassname, klass_ns);
	AddScriptToEntityComponent(entity, world, scriptID);

}

void BehaviourSystem::RegisterComponent(ecs::entity& entity) {
	behaviour& component = Engine::GetWorld().get_component_from_entity<behaviour>(entity);
	auto world = Engine::GetWorld();
	for (auto name : component.classesName) {
		if (component.scriptIDs.size() > 0) continue; //already registered
		std::string className, namespaceName;
		parse_class_name(name, namespaceName, className);
		if (namespaceName.empty()) {
			AddScriptToEntityComponent(entity, world, className.c_str());
		}
		else {

			AddScriptToEntityComponent(entity, world, className.c_str(), namespaceName.c_str());
		}
	}
}

