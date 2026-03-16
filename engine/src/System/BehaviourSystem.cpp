#include "MonoManager.hpp"
#include "System/BehaviourSystem.hpp"
#include "Manager/MonoEntityManager.hpp"
#include "components/behaviour.hpp"
#include "Engine.hpp"
#include "ABI/ABI.h"
#include "Bindings/MANAGED_CONSOLE.hpp"
#include "spdlog/spdlog.h"
#include "Profiler/profiler.hpp"
#include "Scene/Scene.hpp"

#include <algorithm>
#include <mono/jit/jit.h>
#include <mono/metadata/object.h>
#include <sstream>
#include <string_view>

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

	for (auto entity : entities) {
		behaviour& component = world.get_component_from_entity<behaviour>(entity);
		for (int i = 0; i < component.scriptIDs.size(); i++) {
			rp::Guid scriptID = component.scriptIDs[i];
			std::string klassName = component.classesName[i];
			ApplyScriptProperties(component, klassName, scriptID);

		}
	}


}

void BehaviourSystem::Reload()
{
	spdlog::info("BehaviourSystem: Reloading managed behaviour instances");
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

	for (auto entity : entities) {
		behaviour& component = world.get_component_from_entity<behaviour>(entity);
		for (int i = 0; i < component.scriptIDs.size(); i++) {
			rp::Guid scriptID = component.scriptIDs[i];
			std::string klassName = component.classesName[i];
			ApplyScriptProperties(component, klassName, scriptID);

		}
	}



	firstRun = true;
	spdlog::info("BehaviourSystem: Managed behaviour reload complete");
}

void BehaviourSystem::InitScripts(ecs::world& world)
{
	if (!firstRun) return;
	auto entities = world.filter_entities<behaviour>();
	for (auto entity : entities) {
		behaviour& component = world.get_component_from_entity<behaviour>(entity);
		for (auto scriptID : component.scriptIDs) {
			CSKlassInstance* instance = MonoEntityManager::GetInstance().GetInstance(scriptID);
			if (instance) {
				MonoObject* exception = nullptr;
				spdlog::info("First run is invoked for ScriptId: {}", scriptID.to_hex_prefix());
				instance->Invoke("Init", nullptr, &exception, 0);
				if (exception) {
					MonoString* excStr = mono_object_to_string(exception, nullptr);
					ManagedConsole::LogError(excStr);

					auto it = std::find(component.scriptIDs.begin(), component.scriptIDs.end(), scriptID);
					auto instance2 = MonoEntityManager::GetInstance().GetInstance(scriptID);
					if (it != component.scriptIDs.end()) {
						instance2->Reset();
						component.scriptIDs.erase(it);
					}
				}
			}
		}
	}

	firstRun = false;
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
				instance->Invoke("Update", nullptr, &exception, 0);


				if (exception) {
					MonoString* excStr = mono_object_to_string(exception, nullptr);
					ManagedConsole::LogError(excStr);
					// Unload the script instance to prevent further errors
					auto it = std::find(component.scriptIDs.begin(), component.scriptIDs.end(), scriptID);
					auto instance2 = MonoEntityManager::GetInstance().GetInstance(scriptID);
					if (it != component.scriptIDs.end()) {
						instance2->Reset();
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

namespace
{
	bool ParseBool(std::string_view value, bool& outValue)
	{
		if (value == "true" || value == "1")
		{
			outValue = true;
			return true;
		}
		if (value == "false" || value == "0")
		{
			outValue = false;
			return true;
		}
		return false;
	}

	bool ParseVec2(std::string_view value, float& x, float& y)
	{
		std::string tmp(value);
		std::stringstream stream(tmp);
		char comma = '\0';
		if (stream >> x >> comma >> y)
		{
			return comma == ',';
		}
		return false;
	}

	bool ParseVec3(std::string_view value, float& x, float& y, float& z)
	{
		std::string tmp(value);
		std::stringstream stream(tmp);
		char comma1 = '\0';
		char comma2 = '\0';
		if (stream >> x >> comma1 >> y >> comma2 >> z)
		{
			return comma1 == ',' && comma2 == ',';
		}
		return false;
	}

	bool ParseSceneEntityReference(std::string_view value, SceneEntityReference& outRef)
	{
		constexpr std::string_view guidKey = "scene_guid:";
		constexpr std::string_view idKey = "scene_id:";

		const size_t guidPos = value.find(guidKey);
		const size_t idPos = value.find(idKey);
		if (guidPos == std::string_view::npos || idPos == std::string_view::npos)
		{
			return false;
		}

		const size_t guidStart = guidPos + guidKey.size();
		const size_t guidEnd = value.find(';', guidStart);
		if (guidEnd == std::string_view::npos)
		{
			return false;
		}

		const std::string guidStr(value.substr(guidStart, guidEnd - guidStart));
		const std::string idStr(value.substr(idPos + idKey.size()));

		try
		{
			const rp::Guid guid = rp::Guid::to_guid(guidStr);
			const uint32_t sceneId = static_cast<uint32_t>(std::stoul(idStr));
			outRef.m_scene_guid = { guid, rp::utility::compute_string_hash("scene") };
			outRef.m_scene_id = sceneId;
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	bool SetFieldFromString(MonoObject* scriptObject,
		const CSKlass* klass,
		std::string_view fieldName,
		std::string_view typeName,
		std::string_view value,
		bool is_user = false)
	{
		if (!scriptObject || !klass)
		{
			return false;
		}

		const std::string fieldNameStr(fieldName);
		CSKlass::FieldInfo* fieldInfo = const_cast<CSKlass*>(klass)->ResolveField(fieldNameStr.c_str());
		if (!fieldInfo)
		{
			return false;
		}

		if (typeName == "System.Boolean")
		{
			bool parsed = false;
			if (!ParseBool(value, parsed))
			{
				return false;
			}
			mono_bool rawValue = parsed ? 1 : 0;
			mono_field_set_value(scriptObject, fieldInfo->field, &rawValue);
			return true;
		}
		if (typeName == "System.Int32")
		{
			try
			{
				int parsed = std::stoi(std::string(value));
				mono_field_set_value(scriptObject, fieldInfo->field, &parsed);
				return true;
			}
			catch (...)
			{
				return false;
			}
		}
		if (typeName == "System.Single")
		{
			try
			{
				float parsed = std::stof(std::string(value));
				mono_field_set_value(scriptObject, fieldInfo->field, &parsed);
				return true;
			}
			catch (...)
			{
				return false;
			}
		}
		if (typeName == "System.Double")
		{
			try
			{
				double parsed = std::stod(std::string(value));
				mono_field_set_value(scriptObject, fieldInfo->field, &parsed);
				return true;
			}
			catch (...)
			{
				return false;
			}
		}
		if (typeName == "System.String")
		{
			MonoString* monoStr = mono_string_new(mono_domain_get(), std::string(value).c_str());
			mono_field_set_value(scriptObject, fieldInfo->field, monoStr);
			return true;
		}
		if (typeName == "BasilEngine.Mathematics.Vector2")
		{
			float x = 0.0f;
			float y = 0.0f;
			if (!ParseVec2(value, x, y))
			{
				return false;
			}
			struct { float x, y; } vec{ x, y };
			mono_field_set_value(scriptObject, fieldInfo->field, &vec);
			return true;
		}
		if (typeName == "BasilEngine.Mathematics.Vector3")
		{
			float x = 0.0f;
			float y = 0.0f;
			float z = 0.0f;
			if (!ParseVec3(value, x, y, z))
			{
				return false;
			}
			struct { float x, y, z; } vec{ x, y, z };
			mono_field_set_value(scriptObject, fieldInfo->field, &vec);
			return true;
		}
		if (typeName == "BasilEngine.GameObject")
		{
			if (value.empty() || value.find("None") == 0)
			{
				mono_field_set_value(scriptObject, fieldInfo->field, nullptr);
				return true;
			}

			SceneEntityReference ref{};
			if (!ParseSceneEntityReference(value, ref))
			{
				return false;
			}

			auto entityOpt = Engine::GetSceneRegistry().GetReferencedEntity(ref);
			if (!entityOpt.has_value())
			{
				mono_field_set_value(scriptObject, fieldInfo->field, nullptr);
				return true;
			}

			uint64_t nativeID = entityOpt.value().get_uuid();
			CSKlass* gameObjectKlass = MonoEntityManager::GetInstance().GetNamedKlass("GameObject", "BasilEngine");
			if (!gameObjectKlass)
			{
				MonoEntityManager::GetInstance().AddNamedKlass("GameObject", "BasilEngine", true);
				gameObjectKlass = MonoEntityManager::GetInstance().GetNamedKlass("GameObject", "BasilEngine");
			}
			if (!gameObjectKlass)
			{
				return false;
			}

			void* args[1] = { &nativeID };
			CSKlassInstance goInstance = gameObjectKlass->CreateInstance(nullptr, args);
			MonoObject* goObject = goInstance.Object();
			if (!goObject)
			{
				return false;
			}

			mono_field_set_value(scriptObject, fieldInfo->field, goObject);
			return true;
		}
		else if (is_user)
		{
			if (value.empty() || value.find("None") == 0)
			{
				mono_field_set_value(scriptObject, fieldInfo->field, nullptr);
				return true;
			}

			SceneEntityReference ref{};
			if (!ParseSceneEntityReference(value, ref))
			{
				return false;
			}

			auto entityOpt = Engine::GetSceneRegistry().GetReferencedEntity(ref);
			if (!entityOpt.has_value())
			{
				mono_field_set_value(scriptObject, fieldInfo->field, nullptr);
				return true;
			}

			[[maybe_unused]] uint64_t nativeID = entityOpt.value().get_uuid();
			CSKlass* gameObjectKlass = MonoEntityManager::GetInstance().GetNamedKlass("GameObject", "BasilEngine");
			if (!gameObjectKlass)
			{
				MonoEntityManager::GetInstance().AddNamedKlass("GameObject", "BasilEngine", true);
				gameObjectKlass = MonoEntityManager::GetInstance().GetNamedKlass("GameObject", "BasilEngine");
			}
			if (!gameObjectKlass)
			{
				return false;
			}


			std::string kName, KNamespace;
			std::string type = std::string(typeName);
			MonoEntityManager::GetInstance().SplitTypeName(type.c_str(), KNamespace, kName);
			rp::Guid scriptID = BehaviourSystem::Instance().GetScriptIDFromClassName(entityOpt.value(), kName.c_str(), KNamespace.c_str());
			if (!scriptID)
			{
				return false;
			}
			CSKlassInstance* inst = MonoEntityManager::GetInstance().GetInstance(scriptID);
			if (!inst || !inst->IsValid())
			{
				return false;
			}
			MonoObject* userObject = inst->Object();
			if (!userObject)
			{
				return false;
			}
			mono_field_set_value(scriptObject, fieldInfo->field, userObject);
			return true;
		}



		return false;
	}
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
	rp::Guid scriptID = MonoEntityManager::GetInstance().AddInstance(klassname, klass_ns, nullptr, false, entity.get_uuid());
	if (AddScriptToEntityComponent(entity, world, scriptID))
	{
		behaviour& component = world.get_component_from_entity<behaviour>(entity);
		component.scriptIDs.push_back(scriptID);
		std::string managedName = (klass_ns && *klass_ns) ? (std::string(klass_ns) + "." + klassname) : std::string(klassname);

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

void BehaviourSystem::ApplyScriptProperties(behaviour& component, const std::string& managedName, rp::Guid scriptID)
{
	if (!scriptID)
	{
		return;
	}

	CSKlassInstance* instance = MonoEntityManager::GetInstance().GetInstance(scriptID);
	if (!instance || !instance->IsValid())
	{
		return;
	}

	const CSKlass* klass = instance->Klass();
	if (!klass || !klass->IsValid())
	{
		return;
	}

	instance->UpdateManaged();
	MonoObject* scriptObject = instance->Object();
	if (!scriptObject)
	{
		return;
	}

	const std::string prefix = managedName + ".";
	for (const auto& prop : component.scriptProperties)
	{
		if (prop.name.rfind(prefix, 0) != 0)
		{
			continue;
		}

		const std::string fieldName = prop.name.substr(prefix.size());
		SetFieldFromString(scriptObject, klass, fieldName, prop.typeName, prop.value, prop.is_user_type);
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
			case CollisionCallback::OnTriggerEnter:
				inst->Invoke("OnTriggerEnter", argsCollision, nullptr, 1);
				break;
			case CollisionCallback::OnTriggerStay:
				inst->Invoke("OnTriggerStay", argsCollision, nullptr, 1);
				break;
			case CollisionCallback::OnTriggerExit:
				inst->Invoke("OnTriggerExit", argsCollision, nullptr, 1);
				break;
			}
		}
	}
}

rp::Guid BehaviourSystem::GetScriptIDFromClassName(ecs::entity& entity, const char* name, const char* ns)
{
	// Get the class from MonoEntityManager
	CSKlass* klass = MonoEntityManager::GetInstance().GetNamedKlass(name, ns ? ns : "");
	if (!klass || !klass->IsValid())
	{
		return rp::Guid{};
	}
	bool hasBehaviour = entity.all<behaviour>();
	if (!hasBehaviour)
	{
		return rp::Guid{};
	}
	behaviour& component = entity.get<behaviour>();

	for (const auto& scriptID : component.scriptIDs)
	{
		CSKlassInstance* inst = MonoEntityManager::GetInstance().GetInstance(scriptID);
		if (inst)
		{
			const CSKlass* instKlass = inst->Klass();
			if (instKlass == klass)
			{
				return scriptID;
			}
		}
	}
	return rp::Guid{};
}

