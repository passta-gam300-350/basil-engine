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

	m_class = mono_class_from_name(m_image, m_namespace.empty() ? nullptr : m_namespace.c_str(), m_name.c_str());
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

void CSKlassInstance::Attach(const CSKlass* klass, MonoObject* instance, bool isNative)
{
	m_klass = klass;
	m_instance = instance;
	if (instance)
		m_instanceHandle = (isNative) ? mono_gchandle_new(instance, false) : mono_gchandle_new_weakref(instance, false);
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
