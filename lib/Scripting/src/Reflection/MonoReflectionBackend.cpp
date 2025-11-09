/******************************************************************************/
/*!
\file   MonoReflectionBackend.cpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the implmentation for the MonoReflectionBackend class, which
provides an interface for reflecting over managed types and their members in the Mono runtime.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#include "Reflection/MonoReflectionBackend.hpp"
#include <mono/metadata/metadata.h>
#include "ABI/ABI.h"

void MonoReflectionBackend::EnumerateClass(ManagedAssembly* assembly, const ClassEnumerator& enumerator)
{
	if (!assembly || !assembly->IsLoaded())
		return;

	MonoImage* image = assembly->Image();
	if (!image)
		return;

	const MonoTableInfo* typesTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
	if (!typesTable)
		return;

	int rows = mono_table_info_get_rows(typesTable);
	for (int i = 0; i < rows; ++i)
	{
		uint32_t cols[MONO_TYPEDEF_SIZE];
		mono_metadata_decode_row(typesTable, i, cols, MONO_TYPEDEF_SIZE);

		uint32_t nameIdx = cols[MONO_TYPEDEF_NAME];
		uint32_t namespaceIdx = cols[MONO_TYPEDEF_NAMESPACE];

		const char* className = mono_metadata_string_heap(image, nameIdx);
		const char* namespaceName = mono_metadata_string_heap(image, namespaceIdx);

		if (strcmp(className, "<Module>") == 0)
			continue;
		MonoClass* klass = mono_class_get(image, mono_metadata_make_token(MONO_TABLE_TYPEDEF, i + 1));
		enumerator(namespaceName, className, klass, assembly);
	}
}

void MonoReflectionBackend::EnumerateProperty(CSKlass* klass, StaticPropertyEnumerator const& enumerator)
{
	if (!klass || !enumerator)
		return;
	if (!klass->IsValid())
		return;
	MonoClass* monoClass = klass->Klass();
	if (!monoClass)
		return;
	for (void* iter = nullptr; MonoProperty * property = mono_class_get_properties(monoClass, &iter);)
	{
		const char* propertyName = mono_property_get_name(property);
		MonoMethod* getter = mono_property_get_get_method(property);
		if (!getter)
			continue;
		MonoMethodSignature* sig = mono_method_signature(getter);
		MonoType* retType = mono_signature_get_return_type(sig);
		char* typeName = mono_type_get_name(retType);
		enumerator(propertyName, typeName, klass, property);
	}
}

void MonoReflectionBackend::EnumerateProperty(CSKlassInstance* instance, PropertyEnumerator const& enumerator)
{
	if (!instance || !enumerator)
		return;
	const CSKlass* klass = instance->Klass();
	if (!klass || !klass->IsValid())
		return;
	MonoClass* monoClass = klass->Klass();
	if (!monoClass)
		return;
	for (void* iter = nullptr; MonoProperty * property = mono_class_get_properties(monoClass, &iter);)
	{
		const char* propertyName = mono_property_get_name(property);
		MonoMethod* getter = mono_property_get_get_method(property);
		if (!getter)
			continue;
		MonoMethodSignature* sig = mono_method_signature(getter);
		MonoType* retType = mono_signature_get_return_type(sig);
		char* typeName = mono_type_get_name(retType);
		enumerator(propertyName, typeName, instance, property);
	}
}


void MonoReflectionBackend::EnumerateField(CSKlass* klass, StaticFieldEnumerator const& enumerator)
{
	if (!klass || !enumerator)
		return;

	if (!klass->IsValid())
		return;

	MonoClass* monoClass = klass->Klass();
	if (!monoClass)
		return;

	for (void* iter = nullptr; MonoClassField * field = mono_class_get_fields(monoClass, &iter);)
	{
		const char* fieldName = mono_field_get_name(field);
		CSKlass::FieldInfo* fieldInfo = klass->ResolveField(fieldName);
		if (!fieldInfo)
			continue;
		MonoType* fieldType = fieldInfo->type;
		char* typeName = mono_type_get_name(fieldType);
		enumerator(fieldName, typeName, klass, field);

	}
}

void MonoReflectionBackend::EnumerateField(CSKlassInstance* instance, FieldEnumerator const& enumerator)
{
	if (!instance || !enumerator)
		return;
	const CSKlass* klass = instance->Klass();
	if (!klass || !klass->IsValid())
		return;
	MonoClass* monoClass = klass->Klass();
	if (!monoClass)
		return;
	for (void* iter = nullptr; MonoClassField * field = mono_class_get_fields(monoClass, &iter);)
	{
		const char* fieldName = mono_field_get_name(field);
		MonoType* fieldType = mono_field_get_type(field);
		char* typeName = mono_type_get_name(fieldType);
		enumerator(fieldName, typeName, instance, field);
	}
}

void MonoReflectionBackend::EnumerateMethod(CSKlass* klass, StaticMethodEnumerator const& enumerator)
{
	if (!klass || !enumerator)
		return;

	if (!klass->IsValid())
		return;

	MonoClass* monoClass = klass->Klass();
	if (!monoClass)
		return;

	for (void* iter = nullptr; MonoMethod * method = mono_class_get_methods(monoClass, &iter);)
	{
		MonoMethodSignature* sig = mono_method_signature(method);
		MonoType* retType = mono_signature_get_return_type(sig);
		const char* methodName = mono_method_get_name(method);
		char* retTypeName = mono_type_get_name(retType);
		enumerator(methodName, retTypeName, klass, method);

	}
}

void MonoReflectionBackend::EnumerateMethod(CSKlassInstance* instance, MethodEnumerator const& enumerator)
{
	if (!instance || !enumerator)
		return;

	const CSKlass* klass = instance->Klass();
	if (!klass || !klass->IsValid())
		return;

	MonoClass* monoClass = klass->Klass();
	if (!monoClass)
		return;

	for (void* iter = nullptr; MonoMethod * method = mono_class_get_methods(monoClass, &iter);)
	{
		MonoMethodSignature* sig = mono_method_signature(method);
		MonoType* retType = mono_signature_get_return_type(sig);
		const char* methodName = mono_method_get_name(method);
		char* retTypeName = mono_type_get_name(retType);
		enumerator(methodName, retTypeName, instance, method);
	}
}

[[maybe_unused]] void MonoReflectionBackend::EnumerateEvent([[maybe_unused]] CSKlass* klass, [[maybe_unused]]StaticEventEnumerator const& enumerator)
{
	
	return; // Not implemented
}
[[maybe_unused]]  void MonoReflectionBackend::EnumerateEvent([[maybe_unused]]  CSKlassInstance* instance, [[maybe_unused]]EventEnumerator const& enumerator)
{
	return; // Not implemented
}