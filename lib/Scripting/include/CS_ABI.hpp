/******************************************************************************/
/*!
\file   CS_ABI.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contains the declaration for the C# ABI interface
which provides interoperability between C++ and C# using the Mono runtime.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef CS_ABI_HPP
#define CS_ABI_HPP

#include <string>
#include <mono/utils/mono-forward.h>
#include <mono/metadata/class.h>
#include <mono/metadata/object.h>



template <typename T>
concept Numeric = std::is_arithmetic_v<T> && !std::is_same_v<T, bool>;

struct AssemblyInfo
{
	std::string name;
	MonoAssembly* assembly;
	MonoImage* image;
};



struct ManagedClass
{
	AssemblyInfo* assembly_info;

	std::string name;
	MonoClass* klass;

	MonoObject* CreateInstance();
};


struct ManagedObject
{

	size_t handle; //!< GC Handle

	ManagedClass* managed_class;
	MonoObject* object;


	void UpdateManaged();
	MonoObject* Invoke(const char* methodName, void** args=nullptr, MonoObject** exceptionObject=nullptr);

	template <typename T>
	T GetFieldValue(const char* fieldName);
	template <typename T>
	T GetFieldValue(const char* fieldName) requires Numeric<T>;

};


template <typename T>
T ManagedObject::GetFieldValue(const char* fieldName) 
{
	MonoClassField* field = mono_class_get_field_from_name(managed_class->klass, fieldName);
	if (!field) return T();

	T value{};
	mono_field_get_value(object, field, &value);
	return value;
}

template <typename  T>
T ManagedObject::GetFieldValue(const char* fieldName) requires Numeric<T>
{
	MonoClassField* field = mono_class_get_field_from_name(managed_class->klass, fieldName);
	if (!field) return T();
	T value{};
	mono_field_get_value(object, field, &value);
	return value;
}



template <>
std::string ManagedObject::GetFieldValue<std::string>(const char* fieldName)
{
	MonoClassField* field = mono_class_get_field_from_name(managed_class->klass, fieldName);
	if (!field) return std::string();
	MonoString* monoStr = nullptr;
	mono_field_get_value(object, field, &monoStr);
	if (!monoStr) return std::string();
	char* cStr = mono_string_to_utf8(monoStr);
	std::string value(cStr);
	mono_free(cStr);
	return value;
}

template <>
MonoObject* ManagedObject::GetFieldValue<MonoObject*>(const char* fieldName)
{
	MonoClassField* field = mono_class_get_field_from_name(managed_class->klass, fieldName);
	if (!field) return nullptr;
	MonoObject* obj = nullptr;
	mono_field_get_value(object, field, reinterpret_cast<void*>(&obj));
	return obj;
}

#endif // CS_ABI_HPP
