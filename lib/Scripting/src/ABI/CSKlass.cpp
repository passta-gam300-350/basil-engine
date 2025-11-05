#include "ABI/CSKlass.hpp"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-config.h>

#include <utility>
#include <iostream>

CSKlass::CSKlass(MonoImage* image, std::string_view namespaceName, std::string_view className)
{
	Bind(image, namespaceName, className);
}

bool CSKlass::Bind(MonoImage* image, std::string_view namespaceName, std::string_view className)
{
	m_image = image;
	m_namespace.assign(namespaceName.data(), namespaceName.size());
	m_name.assign(className.data(), className.size());

	m_class = nullptr;
	m_methodCache.clear();

	if (!m_image || m_name.empty())
	{
		return false;
	}

	//// Print all classes in the image for debugging
	//MonoClass* assemblyClass = mono_class_from_name(mono_get_corlib(), "System.Reflection", "Assembly");
	//MonoMethod* getTypesMethod = mono_class_get_method_from_name(assemblyClass, "GetTypes", 0);

	//MonoAssembly* assembly = mono_image_get_assembly(m_image);
	//char const* nameAsm = mono_assembly_name_get_name(mono_assembly_get_name(assembly));
	//std::cout << "Listing classes in assembly: " << nameAsm << std::endl;
	//MonoReflectionAssembly* asmObj = mono_assembly_get_object(mono_domain_get(), assembly);
	//MonoArray* typesArray = (MonoArray*)mono_runtime_invoke(getTypesMethod, asmObj, nullptr, nullptr);

	//uintptr_t len = mono_array_length(typesArray);
	//for (int i = 0; i < len; ++i)
	//{
	//	MonoReflectionType* reflType = (MonoReflectionType*)mono_array_get(typesArray, MonoObject*, i);
	//	MonoType* type = mono_reflection_type_get_type(reflType);
	//	MonoClass* klass = mono_class_from_mono_type(type);

	//	const char* ns = mono_class_get_namespace(klass);
	//	const char* name = mono_class_get_name(klass);
	//	std::cout << "Found class: " << ((!std::string(ns).empty()) ? ns : "(global)") << "." << name << std::endl;
	//}

	m_class = mono_class_from_name(m_image, m_namespace.empty() ? "" : m_namespace.c_str(), m_name.c_str());
	if (!m_class)
	{
		std::cerr << "Failed to bind class: " << m_namespace << "." << m_name << std::endl;
		return false;
	}


	return m_class != nullptr;
}

void CSKlass::Reset() noexcept
{
	m_image = nullptr;
	m_class = nullptr;
	m_namespace.clear();
	m_name.clear();
	m_methodCache.clear();
}

bool CSKlass::IsValid() const noexcept
{
	return m_class != nullptr;
}

MonoClass* CSKlass::Klass() const noexcept
{
	return m_class;
}

MonoImage* CSKlass::Image() const noexcept
{
	return m_image;
}

std::string_view CSKlass::Namespace() const noexcept
{
	return m_namespace;
}

std::string_view CSKlass::Name() const noexcept
{
	return m_name;
}

MonoObject* CSKlass::Invoke(const char* methodName, void** args, MonoObject** exception, int paramCount) const
{
	if (!IsValid() || !methodName)
	{
		return nullptr;
	}

	MonoMethod* method = ResolveMethod(methodName, paramCount);
	if (!method)
	{
		return nullptr;
	}

	return mono_runtime_invoke(method, nullptr, args, exception);
}

CSKlassInstance CSKlass::CreateInstance(MonoDomain* domain, void* args[]) const
{
	if (!IsValid())
	{
		return {};
	}

	if (!domain)
	{
		domain = mono_domain_get();
	}

	if (!domain)
	{
		return {};
	}

	MonoObject* object = mono_object_new(domain, m_class);
	if (!object)
	{
		return {};
	}

	if (!args)
		mono_runtime_object_init(object);
	else {
		MonoMethod* constructor = mono_class_get_method_from_name(m_class, ".ctor", args ? -1 : 0);
		if (constructor)
		{
			MonoObject* exception = nullptr;
			mono_runtime_invoke(constructor, object, args, &exception);
			if (exception)
			{
				throw "Exception occurred during object construction.";
			}
		}
	}
	return CSKlassInstance(this, object);
}

CSKlassInstance CSKlass::CreateNativeInstance(MonoDomain* domain) const
{
	if (!IsValid())
	{
		return {};
	}

	if (!domain)
	{
		domain = mono_domain_get();
	}

	if (!domain)
	{
		return {};
	}

	MonoObject* object = mono_object_new(domain, m_class);
	if (!object)
	{
		return {};
	}

	mono_runtime_object_init(object);
	return CSKlassInstance(this, object, true);
}

MonoMethod* CSKlass::GetMethod(const char* methodName, int paramCount) const
{
	return ResolveMethod(methodName, paramCount);
}

CSKlass::FieldInfo* CSKlass::ResolveField(const char* fieldName)
{
	if (!IsValid() || !fieldName)
	{
		return nullptr;
	}
	for (auto& fieldInfo : m_FieldInfos)
	{
		if (strcmp(fieldName, fieldInfo.name) == 0)
		{
			return &fieldInfo;
		}
	}
	MonoClassField* field = mono_class_get_field_from_name(m_class, fieldName);
	if (!field)
	{
		return nullptr;
	}
	MonoType* type = mono_field_get_type(field);
	unsigned offset = mono_field_get_offset(field);
	FieldInfo info;
	info.field = field;
	info.type = type;
	info.name = fieldName;
	info.offset = offset;
	m_FieldInfos.push_back(info);
	return &m_FieldInfos.back();
}

CSKlass::FieldInfo* CSKlass::MakeField(MonoClassField* field)
{
	if (!field)
	{
		return nullptr;
	}

	MonoType* type = mono_field_get_type(field);
	unsigned offset = mono_field_get_offset(field);
	FieldInfo info;
	info.field = field;
	info.type = type;
	info.name = mono_field_get_name(field);
	info.offset = offset;
	m_FieldInfos.push_back(info);
	return &m_FieldInfos.back();
}

MonoMethod* CSKlass::ResolveMethod(const char* methodName, int paramCount) const
{
	if (!methodName || !IsValid())
	{
		return nullptr;
	}

	std::string key = BuildCacheKey(methodName, paramCount);

	auto it = m_methodCache.find(key);
	if (it != m_methodCache.end())
	{
		return it->second;
	}

	MonoMethod* method = mono_class_get_method_from_name(m_class, methodName, paramCount);
	if (method)
	{
		m_methodCache.emplace(std::move(key), method);
	}
	return method;
}

bool CSKlass::IsDerivedFrom(const char* baseClassFullName) const
{
	
	if (!IsValid() || !baseClassFullName)
		return false;

	// Split input into namespace and class name
	std::string baseFull(baseClassFullName);
	std::string baseNamespace;
	std::string baseName;

	MonoClass* parent = mono_class_get_parent(m_class);
	if (!parent) return false;
	

	// Get parent class full name
	while (parent)
	{
		/*MonoImage* img = mono_class_get_image(parent);*/
		const char* parentNamespace = mono_class_get_namespace(parent);
		const char* parentName = mono_class_get_name(parent);
		std::string parentFull = std::string((parentNamespace && *parentNamespace) ? parentNamespace : "") + "." + std::string(parentName);
		if (parentFull == baseFull)
		{
			return true;
		}
		parent = mono_class_get_parent(parent);

	}
	return false;
	
}


bool CSKlass::IsDerivedFrom(const CSKlass& baseClass) const
{
	if (!IsValid() || !baseClass.IsValid())
		return false;
	MonoClass* parent = mono_class_get_parent(m_class);
	if (!parent) return false;
	MonoClass* baseKlass = baseClass.Klass();
	
	if (!baseKlass) return false;
	if (parent == baseKlass)
	{
		return true;
	}
	return false;
}



std::string CSKlass::BuildCacheKey(const char* methodName, int paramCount) const
{
	std::string key;
	key.reserve(std::char_traits<char>::length(methodName) + 8);
	key.append(methodName);
	key.push_back('#');
	key.append(std::to_string(paramCount));
	return key;
}

CSKlassInstance::CSKlassInstance(const CSKlass* klass, bool isNative)
{
	if (klass)
	{
		Attach(klass, nullptr, isNative);
	}
}

CSKlassInstance::CSKlassInstance(const CSKlass* klass, MonoObject* instance, bool isNative)
{
	Attach(klass, instance, isNative);
}

void CSKlassInstance::Attach(const CSKlass* klass, MonoObject* instance, [[maybe_unused]] bool isNative)
{
	m_klass = klass;
	m_instance = instance;
	if (instance)
		m_instanceHandle = (true) ? mono_gchandle_new(instance, false) : mono_gchandle_new_weakref(instance, false);
	else m_instanceHandle = 0;
}

void CSKlassInstance::Reset() noexcept
{
	m_klass = nullptr;
	m_instance = nullptr;
}

bool CSKlassInstance::IsValid() const noexcept
{
	return m_klass && m_klass->IsValid() && m_instance;
}

const CSKlass* CSKlassInstance::Klass() const noexcept
{
	return m_klass;
}

MonoObject* CSKlassInstance::Object() const noexcept
{
	return (m_instanceHandle) ? mono_gchandle_get_target(m_instanceHandle) : nullptr;
}

void CSKlassInstance::UpdateManaged() noexcept
{
	if (IsValid())
	{
		m_instance = Object();
	}
}

MonoObject* CSKlassInstance::Invoke(const char* methodName, void** args, MonoObject** exception, int paramCount) const
{
	if (!IsValid() || !methodName)
	{
		return nullptr;
	}

	MonoMethod* method = m_klass->GetMethod(methodName, paramCount);
	if (!method)
	{
		return nullptr;
	}

	return mono_runtime_invoke(method, m_instance, args, exception);
}


CSKlassInstance::~CSKlassInstance()
{
	if (m_instanceHandle)
	{
		mono_gchandle_free(m_instanceHandle);
	}
}
