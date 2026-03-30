/******************************************************************************/
/*!
\file   MonoImGuiRenderer.cpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the implementation for the MonoImGuiRenderer class, which
provides an interface for rendering ImGui elements for managed types in the editor.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Manager/MonoImGuiRenderer.hpp"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <iomanip>
#include <Manager/MonoReflectionRegistry.hpp>
#include <Manager/MonoEntityManager.hpp>
#include "MonoResolver/MonoTypeDescriptor.hpp"
#include "ABI/CSKlass.hpp"

#include <sstream>
#include <mono/metadata/appdomain.h>

#include <mono/metadata/object.h>
#include <mono/metadata/metadata.h>

#include "Scene/Scene.hpp"

namespace
{
	const MonoTypeDescriptor* GetElementDescriptor(const FieldNode& node)
	{
		return node.descriptor ? node.descriptor->elementDescriptor : nullptr;
	}
}

void MonoImGuiRenderer::SplitManagedName(const std::string& fullName, std::string& namespaceName, std::string& className)
{
	const auto pos = fullName.rfind('.');
	if (pos == std::string::npos)
	{
		namespaceName.clear();
		className = fullName;
	}
	else
	{
		namespaceName = fullName.substr(0, pos);
		className = fullName.substr(pos + 1);
	}
}

CSKlass* MonoImGuiRenderer::ResolveManagedClass(const std::string& fullName)
{
	std::string namespaceName;
	std::string className;
	SplitManagedName(fullName, namespaceName, className);
	return MonoEntityManager::GetInstance().GetNamedKlass(className.c_str(), namespaceName.c_str());
}

bool MonoImGuiRenderer::RenderField(const FieldNode& fieldNode, CSKlass* klass, CSKlassInstance* instance)
{
	ImGui::PushID(fieldNode.name.c_str());

	const char* label = fieldNode.name.c_str();
	bool modified = false;

	if (!fieldNode.descriptor)
	{
		ImGui::TextDisabled("%s (descriptor unavailable)", label);
		ImGui::PopID();
		return false;
	}

	if (fieldNode.isStatic)
	{
		ImGui::TextDisabled("%s (static field)", label);
		ImGui::PopID();
		return false;
	}

	if (!klass || !klass->IsValid())
	{
		ImGui::TextDisabled("%s (klass unavailable)", label);
		ImGui::PopID();
		return false;
	}

	if (!instance || !instance->IsValid())
	{
		ImGui::TextDisabled("%s (instance unavailable)", label);
		ImGui::PopID();
		return false;
	}

	CSKlass::FieldInfo* fieldInfo = klass->ResolveField(fieldNode.name.c_str());
	if (!fieldInfo)
	{
		ImGui::TextDisabled("%s (field not resolved)", label);
		ImGui::PopID();
		return false;
	}

	MonoObject* scriptObject = instance->Object();
	if (!scriptObject)
	{
		instance->UpdateManaged();
		scriptObject = instance->Object();
	}
	if (!scriptObject)
	{
		ImGui::TextDisabled("%s (instance object unavailable)", label);
		ImGui::PopID();
		return false;
	}

	switch (fieldNode.descriptor->managedKind)
	{
	case ManagedKind::System_Boolean:
	{
		mono_bool rawValue = 0;
		mono_field_get_value(scriptObject, fieldInfo->field, &rawValue);
		bool value = rawValue != 0;
		if (ImGui::Checkbox(label, &value))
		{
			rawValue = value ? 1 : 0;
			mono_field_set_value(scriptObject, fieldInfo->field, &rawValue);
			modified = true;
		}
		break;
	}
	case ManagedKind::System_Int32:
	{
		int value = 0;
		mono_field_get_value(scriptObject, fieldInfo->field, &value);
		if (ImGui::InputInt(label, &value))
		{
			mono_field_set_value(scriptObject, fieldInfo->field, &value);
			modified = true;
		}
		break;
	}
	case ManagedKind::System_Single:
	{
		float value = 0.0f;
		mono_field_get_value(scriptObject, fieldInfo->field, &value);
		if (ImGui::InputFloat(label, &value))
		{
			mono_field_set_value(scriptObject, fieldInfo->field, &value);
			modified = true;
		}
		break;
	}
	case ManagedKind::System_Double:
	{
		double value = 0.0;
		mono_field_get_value(scriptObject, fieldInfo->field, &value);
		if (ImGui::InputDouble(label, &value))
		{
			mono_field_set_value(scriptObject, fieldInfo->field, &value);
			modified = true;
		}
		break;
	}
	case ManagedKind::System_String:
	{
		MonoString* monoStr = nullptr;
		mono_field_get_value(scriptObject, fieldInfo->field, &monoStr);
			
			static std::string value{};
		if (monoStr)
		{
			char* utf8 = mono_string_to_utf8(monoStr);
			if (utf8)
			{
				value = utf8;
				mono_free(utf8);
			}
		}

			bool onInput = ImGui::InputText(label, &value, ImGuiInputTextFlags_EnterReturnsTrue);
			if (onInput)
			{
				MonoDomain* domain = mono_object_get_domain(scriptObject);
				MonoString* newMonoStr = mono_string_new(domain, value.c_str());
				spdlog::info("Set value for script: {}", value);
				mono_field_set_value(scriptObject, fieldInfo->field, newMonoStr);
				modified = true;
			}
		
		

		ImGui::Text("%s: %s", label, value.c_str());
		break;
	}
	case ManagedKind::System_List:
	{
		ImGui::TextDisabled("%s (List type not yet supported)", label);
		break;
	}


	default:
		if (fieldNode.descriptor->managedKind == ManagedKind::Custom)
		{
			auto managed_name = fieldNode.descriptor->managed_name;
			if (managed_name == "BasilEngine.Mathematics.Vector3")
			{


				struct { float x, y, z; } value{};
				mono_field_get_value(scriptObject, fieldInfo->field, &value);

				float vals[3] = { value.x, value.y, value.z };
				if (ImGui::InputFloat3(label, vals))
				{
					value.x = vals[0];
					value.y = vals[1];
					value.z = vals[2];
					mono_field_set_value(scriptObject, fieldInfo->field, &value);
					modified = true;
				}

			}
			else if (managed_name == "BasilEngine.Mathematics.Vector4")
			{
				struct { float x, y, z, w; } value{};
				mono_field_get_value(scriptObject, fieldInfo->field, &value);

				float vals[4] = { value.x, value.y, value.z, value.w };
				if (ImGui::InputFloat4(label, vals))
				{
					value.x = vals[0];
					value.y = vals[1];
					value.z = vals[2];
					value.w = vals[3];
					mono_field_set_value(scriptObject, fieldInfo->field, &value);
					modified = true;
				}
			}
			else if (managed_name == "BasilEngine.Mathematics.Vector2")
			{


				struct { float x, y; } value{};
				mono_field_get_value(scriptObject, fieldInfo->field, &value);

				float vals[2] = { value.x, value.y };
				if (ImGui::InputFloat2(label, vals))
				{
					value.x = vals[0];
					value.y = vals[1];

					mono_field_set_value(scriptObject, fieldInfo->field, &value);
					modified = true;
				}

				break;
			}
			else if (managed_name == "BasilEngine.Rendering.Color")
			{
				struct { float r, g, b, a; } value{};
				mono_field_get_value(scriptObject, fieldInfo->field, &value);

				float vals[4] = { value.r, value.g, value.b, value.a };
				if (ImGui::ColorEdit4(label, vals, ImGuiColorEditFlags_PickerHueWheel))
				{
					value.r = vals[0];
					value.g = vals[1];
					value.b = vals[2];
					value.a = vals[3];
					mono_field_set_value(scriptObject, fieldInfo->field, &value);
					modified = true;
				}
			}
			else if (managed_name == "BasilEngine.Rendering.Color32")
			{
				struct { unsigned char r, g, b, a; } value{};
				mono_field_get_value(scriptObject, fieldInfo->field, &value);

				float vals[4] = {
					value.r / 255.0f,
					value.g / 255.0f,
					value.b / 255.0f,
					value.a / 255.0f
				};
				if (ImGui::ColorEdit4(label, vals, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_Uint8))
				{
					value.r = static_cast<unsigned char>(std::clamp(vals[0], 0.0f, 1.0f) * 255.0f);
					value.g = static_cast<unsigned char>(std::clamp(vals[1], 0.0f, 1.0f) * 255.0f);
					value.b = static_cast<unsigned char>(std::clamp(vals[2], 0.0f, 1.0f) * 255.0f);
					value.a = static_cast<unsigned char>(std::clamp(vals[3], 0.0f, 1.0f) * 255.0f);
					mono_field_set_value(scriptObject, fieldInfo->field, &value);
					modified = true;
				}
			}
			else if (fieldNode.descriptor->managed_name == "BasilEngine.GameObject")
			{
				RenderGameObjectField(fieldNode, klass, instance, fieldInfo);
			}
			else if (fieldNode.descriptor->isUserType)
			{
				//ImGui::TextDisabled("%s (user-defined(%s) type not yet supported)", label, fieldNode.descriptor->managed_name.c_str());
				RenderUserObjectField(fieldNode, klass, instance, fieldInfo);
			}
		}

		else
		{
			ImGui::TextDisabled("%s (type unknown)", label);
		}

		break;
	}

	ImGui::PopID();
	return modified;
}

void MonoImGuiRenderer::RenderGameObjectField(const FieldNode& fieldNode, [[maybe_unused]] CSKlass* klass, CSKlassInstance* instance, CSKlass::FieldInfo* info)
{

	MonoObject* gameObjRef = nullptr;
	MonoObject* inst = instance->Object();


	CSKlass* nativeClass = MonoEntityManager::GetInstance().GetNamedKlass("NativeObject", "BasilEngine");

	mono_field_get_value(inst, info->field, &gameObjRef);

	//Get Native Object csharp class
	//CSKlass* NativeClass = MonoEntityManager::GetInstance().GetNamedKlass("NativeObject", "BasilEngine");

	uint64_t nativeID = 0;
	//Get NativeID field
	CSKlass::FieldInfo* nativeIDField = nativeClass->ResolveField("NativeID");
	SceneEntityReference goReference{};
	if (gameObjRef)
	{
		mono_field_get_value(gameObjRef, nativeIDField->field, &nativeID);
	}




	std::string selected = "None (GameObject)";



	std::vector<std::pair<const char*, SceneEntityReference>> entities_ref{};

	auto get_all_entities_ref = [&entities_ref]()
	{
		auto world = Engine::GetWorld();
		auto entities = world.get_all_entities();
		for (auto& entity : entities)
		{
			SceneEntityReference entityRef{};
			entityRef.m_scene_guid = { Engine::GetSceneRegistry().GetActiveSceneGuid(), rp::utility::compute_string_hash("scene") };
			entityRef.m_scene_id = entity.get_scene_uid();
			entities_ref.push_back({ entity.name().c_str(), entityRef });
		}
	};


	if (ImGui::BeginPopup("MENU_ENTITY_LIST")) {
		// List all entities

		get_all_entities_ref();
		int counter = 0;
		for (auto& entity : entities_ref)
		{
			// Get entity name

			std::string const& entityName = entity.first;

			std::string uuid = std::to_string(counter++) + "_ENTITY_LIST";
			// Display entity as selectable
			ImGui::PushID(uuid.c_str());
			if (ImGui::Selectable(entityName.c_str()))
			{
				// Do some C# operations to set the GameObject reference
				CSKlass* gameObjectKlass = MonoEntityManager::GetInstance().GetNamedKlass("GameObject", "BasilEngine");
				const SceneEntityReference ref = entity.second;
				auto selected_entity = Engine::GetSceneRegistry().GetReferencedEntity(ref);
				if (selected_entity.has_value())
				{
					ecs::entity e = selected_entity.value();
					uint64_t nativeIDValue = e.get_uuid();
					void* data[1] = { &nativeIDValue };

					CSKlassInstance goInstance = gameObjectKlass->CreateInstance(nullptr, data);
					MonoObject* goObject = goInstance.Object();
					if (!goObject)
					{
						ImGui::PopID();
						break;
					}
					//Set NativeID field
					CSKlass::FieldInfo* nativeIDField1 = gameObjectKlass->ResolveField("NativeID");
					mono_field_set_value(goObject, nativeIDField1->field, &nativeIDValue);
					//Set GameObject field in the script instance
					mono_field_set_value(inst, info->field, goObject);
					nativeID = nativeIDValue;
				}




				ImGui::PopID();
				break;

			}


			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Select %s (ID: %d)", entityName.c_str(), entity.second.m_scene_id);
			}
			ImGui::PopID();
		}

		ImGui::EndPopup();
	}

	if (nativeID != 0)

	{
		ecs::entity native{ nativeID };
		selected = native.name() + " (GameObject)";
	}
	ImGui::Text("%s : %s ", fieldNode.name.c_str(), selected.c_str());
	ImGui::SameLine();
	bool button = ImGui::Button("Select Entity");
	if (button)
	{
		ImGui::OpenPopup("MENU_ENTITY_LIST");
	}





}

void MonoImGuiRenderer::RenderUserObjectField(const FieldNode& fieldNode, [[maybe_unused]] CSKlass* klass, CSKlassInstance* instance, CSKlass::FieldInfo* info)
{
	MonoObject* gameObjRef = nullptr;
	MonoObject* inst = instance->Object();


	CSKlass* nativeClass = MonoEntityManager::GetInstance().GetNamedKlass("NativeObject", "BasilEngine");

	mono_field_get_value(inst, info->field, &gameObjRef);



	uint64_t nativeID = 0;
	//Get NativeID field
	CSKlass::FieldInfo* nativeIDField = nativeClass->ResolveField("NativeID");
	SceneEntityReference goReference{};
	if (gameObjRef)
	{
		mono_field_get_value(gameObjRef, nativeIDField->field, &nativeID);
	}




	std::string selected = "None (" + fieldNode.descriptor->managed_name + ")";



	std::vector<std::pair<const char*, SceneEntityReference>> entities_ref{};

	auto get_all_entities_ref = [&entities_ref]()
	{
		auto world = Engine::GetWorld();
		auto entities = world.get_all_entities();
		for (auto& entity : entities)
		{
			SceneEntityReference entityRef{};
			entityRef.m_scene_guid = { Engine::GetSceneRegistry().GetActiveSceneGuid(), rp::utility::compute_string_hash("scene") };
			entityRef.m_scene_id = entity.get_scene_uid();
			entities_ref.push_back({ entity.name().c_str(), entityRef });
		}
	};


	if (ImGui::BeginPopup("MENU_ENTITY_LIST_USER")) {
		// List all entities

		get_all_entities_ref();
		int counter = 0;
		for (auto& entity : entities_ref)
		{
			// Get entity name

			std::string const& entityName = entity.first;

			std::string uuid = std::to_string(counter++) + "_ENTITY_LIST_USER";
			// Display entity as selectable
			ImGui::PushID(uuid.c_str());
			if (ImGui::Selectable(entityName.c_str()))
			{
				bool success = false;
				// Do some C# operations to set the GameObject reference
				std::string managedKlassName, managedNamespace;
				SplitManagedName(fieldNode.descriptor->managed_name, managedNamespace, managedKlassName);
				if (managedKlassName.empty())
				{
					ImGui::PopID();
					break;
				}

				if (managedNamespace.empty())
				{
					managedNamespace = "";
				}
				const SceneEntityReference ref = entity.second;
				auto selected_entity = Engine::GetSceneRegistry().GetReferencedEntity(ref);
				if (selected_entity.has_value())
				{


					ecs::entity e = selected_entity.value();


					rp::Guid id = BehaviourSystem::Instance().GetScriptIDFromClassName(e, managedKlassName.c_str(), managedNamespace.empty() ? nullptr : managedNamespace.c_str());
					CSKlassInstance* objInst = MonoEntityManager::GetInstance().GetInstance(id);
					// Set Field in the script instance
					if (objInst && objInst->IsValid())
					{
						mono_field_set_value(inst, info->field, (objInst->Object()));
						mono_field_get_value(objInst->Object(), nativeIDField->field, &nativeID);
						success = true;
					}

				}




				if (!success)
				{
					spdlog::warn("Failed to assign {} to field {}. The selected entity does not have a component of type {}.{}", entityName.c_str(), fieldNode.name.c_str(), managedNamespace.c_str(), managedKlassName.c_str());


					ImGui::PopID();
					continue;
				}
				ImGui::PopID();
				break;

			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Select %s (ID: %d)", entityName.c_str(), entity.second.m_scene_id);
			}
			ImGui::PopID();
		}

		ImGui::EndPopup();
	}

	if (nativeID != 0)

	{
		ecs::entity native{ nativeID };
		selected = native.name() + "(" + fieldNode.descriptor->managed_name + ")";
	}
	ImGui::Text("%s : %s ", fieldNode.name.c_str(), selected.c_str());
	ImGui::SameLine();
	bool button = ImGui::Button("Select Entity");
	if (button)
	{
		ImGui::OpenPopup("MENU_ENTITY_LIST_USER");
	}

}


bool MonoImGuiRenderer::TryGetFieldValueString(const FieldNode& fieldNode,
	CSKlass* klass,
	CSKlassInstance* instance,
	std::string& outValue)
{
	if (!fieldNode.descriptor)
	{
		return false;
	}

	if (fieldNode.isStatic)
	{
		return false;
	}

	if (!klass || !klass->IsValid())
	{
		return false;
	}

	if (!instance || !instance->IsValid())
	{
		return false;
	}

	CSKlass::FieldInfo* fieldInfo = klass->ResolveField(fieldNode.name.c_str());
	if (!fieldInfo)
	{
		return false;
	}

	MonoObject* scriptObject = instance->Object();
	if (!scriptObject)
	{
		instance->UpdateManaged();
		scriptObject = instance->Object();
	}
	if (!scriptObject)
	{
		return false;
	}

	switch (fieldNode.descriptor->managedKind)
	{
	case ManagedKind::System_Boolean:
	{
		mono_bool rawValue = 0;
		mono_field_get_value(scriptObject, fieldInfo->field, &rawValue);
		outValue = (rawValue != 0) ? "true" : "false";
		return true;
	}
	case ManagedKind::System_Int32:
	{
		int value = 0;
		mono_field_get_value(scriptObject, fieldInfo->field, &value);
		outValue = std::to_string(value);
		return true;
	}
	case ManagedKind::System_Single:
	{
		float value = 0.0f;
		mono_field_get_value(scriptObject, fieldInfo->field, &value);
		std::ostringstream stream;
		stream << std::setprecision(7) << value;
		outValue = stream.str();
		return true;
	}
	case ManagedKind::System_Double:
	{
		double value = 0.0;
		mono_field_get_value(scriptObject, fieldInfo->field, &value);
		std::ostringstream stream;
		stream << std::setprecision(15) << value;
		outValue = stream.str();
		return true;
	}
	case ManagedKind::System_String:
	{
		MonoString* monoStr = nullptr;
		mono_field_get_value(scriptObject, fieldInfo->field, &monoStr);

		if (!monoStr)
		{
			outValue.clear();
			return true;
		}

		char* utf8 = mono_string_to_utf8(monoStr);
		if (utf8)
		{
			outValue = utf8;
			mono_free(utf8);
			return true;
		}
		return false;
	}
	case ManagedKind::System_List:
		return false;
	default:
		if (fieldNode.descriptor->managedKind == ManagedKind::Custom)
		{
			auto managed_name = fieldNode.descriptor->managed_name;
			if (managed_name == "BasilEngine.Mathematics.Vector3")
			{
				struct { float x, y, z; } value{};
				mono_field_get_value(scriptObject, fieldInfo->field, &value);

				std::ostringstream stream;
				stream << std::setprecision(7) << value.x << "," << value.y << "," << value.z;
				outValue = stream.str();
				return true;
			}
			else if (managed_name == "BasilEngine.Mathematics.Vector4")
			{
				struct { float x, y, z, w; } value{};
				mono_field_get_value(scriptObject, fieldInfo->field, &value);

				std::ostringstream stream;
				stream << std::setprecision(7) << value.x << "," << value.y << "," << value.z << "," << value.w;
				outValue = stream.str();
				return true;
			}
			else if (managed_name == "BasilEngine.Mathematics.Vector2")
			{
				struct { float x, y; } value{};
				mono_field_get_value(scriptObject, fieldInfo->field, &value);

				std::ostringstream stream;
				stream << std::setprecision(7) << value.x << "," << value.y;
				outValue = stream.str();
				return true;
			}
			else if (managed_name == "BasilEngine.Rendering.Color")
			{
				struct { float r, g, b, a; } value{};
				mono_field_get_value(scriptObject, fieldInfo->field, &value);

				std::ostringstream stream;
				stream << std::setprecision(7) << value.r << "," << value.g << "," << value.b << "," << value.a;
				outValue = stream.str();
				return true;
			}
			else if (managed_name == "BasilEngine.Rendering.Color32")
			{
				struct { unsigned char r, g, b, a; } value{};
				mono_field_get_value(scriptObject, fieldInfo->field, &value);

				std::ostringstream stream;
				stream << static_cast<int>(value.r) << ","
					   << static_cast<int>(value.g) << ","
					   << static_cast<int>(value.b) << ","
					   << static_cast<int>(value.a);
				outValue = stream.str();
				return true;
			}
			else if (managed_name == "BasilEngine.GameObject")
			{
				auto sceneRefString = [](SceneEntityReference const& ref)
				{
					return "scene_guid:" + ref.m_scene_guid.m_guid.to_hex() +
						";scene_id:" + std::to_string(ref.m_scene_id);
				};


				MonoObject* obj = nullptr;
				mono_field_get_value(scriptObject, fieldInfo->field, &obj);

				if (!obj)
				{
					outValue = "None (GameObject)";
					return false;
				}
				else
				{
					SceneEntityReference goRef{};
					CSKlass* goKlass = MonoEntityManager::GetInstance().GetNamedKlass("GameObject", "BasilEngine");
					CSKlass::FieldInfo* nativeIDField = goKlass->ResolveField("NativeID");
					uint64_t nativeID = 0;
					mono_field_get_value(obj, nativeIDField->field, &nativeID);
					ecs::entity native{ nativeID };
					goRef.m_scene_guid = { Engine::GetSceneRegistry().GetActiveSceneGuid(), rp::utility::compute_string_hash("scene") };
					goRef.m_scene_id = native.get_scene_uid();

					outValue = sceneRefString(goRef);
					return true;
				}
			}
			else if (fieldNode.descriptor->isUserType)
			{
				MonoObject* obj = nullptr;
				mono_field_get_value(scriptObject, fieldInfo->field, &obj);
				auto sceneRefString = [](SceneEntityReference const& ref)
				{
					return "scene_guid:" + ref.m_scene_guid.m_guid.to_hex() +
						";scene_id:" + std::to_string(ref.m_scene_id);
				};
				if (!obj)
				{
					outValue = "None (" + fieldNode.descriptor->managed_name + ")";
					return false;
				}

				else
				{
					SceneEntityReference goRef{};
					std::string cKlassName, cNamespace;
					SplitManagedName(fieldNode.descriptor->managed_name, cNamespace, cKlassName);
					CSKlass* userKlass = MonoEntityManager::GetInstance().GetNamedKlass(cKlassName.c_str(), cNamespace.empty() ? "" : cNamespace.c_str());
					if (!userKlass)
					{
						outValue = "Invalid Class (" + fieldNode.descriptor->managed_name + ")";
						return false;
					}

					CSKlass::FieldInfo* nativeIDField = userKlass->ResolveField("NativeID");
					uint64_t nativeID = 0;
					mono_field_get_value(obj, nativeIDField->field, &nativeID);
					if (nativeID == 0)
					{
						outValue = "Invalid NativeID (" + fieldNode.descriptor->managed_name + ")";
						return false;
					}
					ecs::entity native{ nativeID };
					goRef.m_scene_guid = { Engine::GetSceneRegistry().GetActiveSceneGuid(), rp::utility::compute_string_hash("scene") };
					goRef.m_scene_id = native.get_scene_uid();
					outValue = sceneRefString(goRef);
					return true;
				}
			}
		}
		return false;
	}
}

bool MonoImGuiRenderer::RenderBehaviourFields(const std::string& managedName,
	rp::Guid scriptGuid,
	std::vector<ScriptProperty>& outProperties)
{
	const auto* classNode = MonoReflectionRegistry::Instance().FindClassByManagedName(managedName);
	if (!classNode)
	{
		ImGui::TextDisabled("Reflection data not available for %s", managedName.c_str());
		return false;
	}

	CSKlass* klass = ResolveManagedClass(managedName);
	if (!klass || !klass->IsValid())
	{
		ImGui::TextDisabled("Class %s is not loaded", managedName.c_str());
		return false;
	}

	CSKlassInstance* instance = nullptr;
	if (scriptGuid)
	{
		instance = MonoEntityManager::GetInstance().GetInstance(scriptGuid);
	}
	if (instance)
	{
		instance->UpdateManaged();
	}
	assert(instance->IsValid() && "Is valid should be true here!");
	const std::string guidLabel = instance ? scriptGuid.to_hex() : std::string("<unassigned>");
	ImGui::TextDisabled("Script GUID: %s", guidLabel.c_str());

	bool modified = false;
	const std::string namePrefix = managedName + ".";

	for (const auto& [fieldName, fieldNodePtr] : classNode->fields)
	{
		if (!fieldNodePtr)
		{
			continue;
		}

		const FieldNode& fieldNode = *fieldNodePtr;
		if (!fieldNode.isPublic)
		{
			continue;
		}

		modified |= RenderField(fieldNode, klass, instance);

		std::string valueStr;
		if (TryGetFieldValueString(fieldNode, klass, instance, valueStr))
		{
			ScriptProperty prop{};
			prop.name = namePrefix + fieldNode.name;
			prop.typeName = fieldNode.descriptor ? fieldNode.descriptor->managed_name : fieldNode.type;
			prop.value = std::move(valueStr);
			prop.is_user_type = fieldNode.descriptor ? fieldNode.descriptor->isUserType : false;
			outProperties.push_back(std::move(prop));
		}

		if (const MonoTypeDescriptor* elementDescriptor = GetElementDescriptor(fieldNode))
		{
			ImGui::Indent();
			ImGui::TextDisabled("Element Type: %s", elementDescriptor->managed_name.c_str());
			ImGui::Unindent();
		}
	}

	return modified;
}
