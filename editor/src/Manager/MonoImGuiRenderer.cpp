#include "Manager/MonoImGuiRenderer.hpp"

#include "imgui.h"

#include <Manager/MonoReflectionRegistry.hpp>
#include <Manager/MonoEntityManager.hpp>
#include "MonoResolver/MonoTypeDescriptor.hpp"
#include "ABI/CSKlass.hpp"

#include <mono/metadata/object.h>
#include <mono/metadata/metadata.h>

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

		std::string value;
		if (monoStr)
		{
			char* utf8 = mono_string_to_utf8(monoStr);
			if (utf8)
			{
				value = utf8;
				mono_free(utf8);
			}
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

				break;
			} else if (managed_name == "BasilEngine.Mathematics.Vector2")
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
		}
		else {
			ImGui::TextDisabled("%s (type %s not supported)", label, fieldNode.descriptor->managed_name.c_str());
		}
		break;
	}

	ImGui::PopID();
	return modified;
}

void MonoImGuiRenderer::RenderBehaviourFields(const std::string& managedName, rp::Guid scriptGuid)
{
	const auto* classNode = MonoReflectionRegistry::Instance().FindClassByManagedName(managedName);
	if (!classNode)
	{
		ImGui::TextDisabled("Reflection data not available for %s", managedName.c_str());
		return;
	}

	CSKlass* klass = ResolveManagedClass(managedName);
	if (!klass || !klass->IsValid())
	{
		ImGui::TextDisabled("Class %s is not loaded", managedName.c_str());
		return;
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

		RenderField(fieldNode, klass, instance);

		if (const MonoTypeDescriptor* elementDescriptor = GetElementDescriptor(fieldNode))
		{
			ImGui::Indent();
			ImGui::TextDisabled("Element Type: %s", elementDescriptor->managed_name.c_str());
			ImGui::Unindent();
		}
	}
}
