#include "MonoManager.hpp"
#include "System/BehaviourSystem.hpp"
#include "Manager/MonoEntityManager.hpp"
#include "components/behaviour.hpp"
#include "Engine.hpp"
#include "ABI/ABI.h"
#include "Bindings/MANAGED_CONSOLE.hpp"
#include "spdlog/spdlog.h"
#include "Profiler/profiler.hpp"

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
	MonoEntityManager::GetInstance().Attach(); 

	auto world = Engine::GetWorld();

	auto entities = world.filter_entities<behaviour>();
	for (auto entity : entities) {
		behaviour& component = world.get_component_from_entity<behaviour>(entity);
		component.scriptIDs.clear();
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

void BehaviourSystem::Reload()
{
	auto world = Engine::GetWorld();

	auto entities = world.filter_entities<behaviour>();
	for (auto entity : entities) {
		behaviour& component = world.get_component_from_entity<behaviour>(entity);
		component.scriptIDs.clear();
		for (auto val : component.classesName) {
			std::string className, namespaceName;
			parse_class_name(val, namespaceName, className);

			if (namespaceName.empty()) {
				AddScriptToEntityComponent(entity, world, className.c_str());
			}
			else AddScriptToEntityComponent(entity, world, className.c_str(), namespaceName.c_str());
		}
	}

	firstRun = true;
}

void BehaviourSystem::Update(ecs::world& world, float)
{
	if (!isActive) return;
	PF_SYSTEM("Behaviour System");

	auto entites = world.filter_entities<behaviour>();

	if (entites.empty()) return;
	
	for (auto entity : entites) {
		behaviour& component = world.get_component_from_entity<behaviour>(entity);
		for (auto scriptID : component.scriptIDs) {
			CSKlassInstance* instance = MonoEntityManager::GetInstance().GetInstance(scriptID);
			if (instance) {
				MonoObject* exception = nullptr;
				if (firstRun)
				{
					instance->Invoke("Init", nullptr, &exception, 0);
					spdlog::info("First run is executed for script {}", scriptID.to_hex());
				}
				else instance->Invoke("Update", nullptr, &exception, 0);


					if (exception) {
					MonoString* excStr = mono_object_to_string(exception, nullptr);
					ManagedConsole::LogError(excStr);
					// Unload the script instance to prevent further errors
					auto it = std::find(component.scriptIDs.begin(), component.scriptIDs.end(), scriptID);
					auto instance = MonoEntityManager::GetInstance().GetInstance(scriptID);
					if (it != component.scriptIDs.end()) {
						instance->Reset();
						component.scriptIDs.erase(it);
					}
					
					
				}

				/*if (unloaded)
				{
					unloaded = false;
					return;
				}*/
			}
		}

	}

	firstRun = false;
	
}
void BehaviourSystem::FixedUpdate(ecs::world& world)
{
	if (!isActive) return;
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

bool BehaviourSystem::AddScriptToEntityComponent(ecs::entity& /*entity*/, ecs::world& /*world*/, rp::Guid scriptID) {
	//behaviour& component = world.get_component_from_entity<behaviour>(entity);



	CSKlassInstance* instance = MonoEntityManager::GetInstance().GetInstance(scriptID);
	
	if (!instance || !instance->IsValid()) {
		spdlog::warn("[BehaviourSystem] Failed to create instance for script ID: {}", scriptID.to_hex());
		return false;
	}
	const CSKlass* klass = instance->Klass();
	auto derivedClass = MonoEntityManager::GetInstance().GetNamedKlass("Behavior", "BasilEngine.Components");
	if (!derivedClass) {
		MonoEntityManager::GetInstance().AddNamedKlass("Behavior", "BasilEngine.Components", true);
		derivedClass = MonoEntityManager::GetInstance().GetNamedKlass("Behavior", "BasilEngine.Components");
		if (!derivedClass) {
			spdlog::warn("[BehaviourSystem] BasilEngine.Components.Behaviour class not found in assembly!");
			return false;
		}
	}
	bool derived = klass->IsDerivedFrom(*derivedClass);
	if (!derived) {
		spdlog::warn("[BehaviorSystem] Script ID: {} is not derived from BasilEngine.Behavior", scriptID.to_hex());
		return false;
	}


	return true;

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
	bool derived = klass->IsDerivedFrom("BasilEngine.Components.Behavior");
	if (!derived) {
		spdlog::warn("[BehaviourSystem] Class {}.{} is not derived from BasilEngine.Behaviour", klass_ns, klassname);
		return;
	}
	rp::Guid scriptID = MonoEntityManager::GetInstance().AddInstance(klassname, klass_ns, nullptr, false,entity.get_uuid());
	if (AddScriptToEntityComponent(entity, world, scriptID))
	{
		behaviour& component = world.get_component_from_entity<behaviour>(entity);
		component.scriptIDs.push_back(scriptID);
	}

}

void BehaviourSystem::RegisterComponent(ecs::entity& entity) {
	behaviour& component = Engine::GetWorld().get_component_from_entity<behaviour>(entity);
	auto world = Engine::GetWorld();

	component.scriptIDs.clear();
	for (const auto& name : component.classesName) {
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

void BehaviourSystem::OnCollisionCallback(ecs::entity& entity, ecs::entity other, enum CollisionCallback callback)
{
	behaviour& component = entity.get<behaviour>();

	auto GameObjectKlass = MonoEntityManager::GetInstance().GetNamedKlass("GameObject", "BasilEngine");


	void* args[1];
	args[0] = &other;
	auto monoOther = GameObjectKlass->CreateInstance(nullptr, args);

	void* argsCollision[1];
	argsCollision[0] = &monoOther;
	for (auto ScriptID : component.scriptIDs)
	{
		CSKlassInstance* inst = MonoEntityManager::GetInstance().GetInstance(ScriptID);
		if (inst)
		{
			switch (callback)
			{
			case CollisionCallback::OnCollisionEnter:
				inst->Invoke("OnCollisionEnter", argsCollision, nullptr, 1);
				break;
			case CollisionCallback::OnCollisionStay:
				inst->Invoke("OnCollisionStay", argsCollision, nullptr, 1);
				break;
			case CollisionCallback::OnCollisionExit:
				inst->Invoke("OnCollisionExit", argsCollision, nullptr, 1);
				break;
			 
			}
		}
	}
}


