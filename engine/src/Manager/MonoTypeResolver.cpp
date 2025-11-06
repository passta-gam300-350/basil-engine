#include "Manager/MonoTypeResolver.hpp"
#include "MonoResolver/MonoTypeDescriptor.hpp"
#include "Resolver/MonoResolver.hpp"

#include <mono/metadata/metadata.h>
#include <mono/metadata/object.h>
#include <mono/metadata/class.h>

#include <array>
#include <cctype>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>

namespace
{
	std::string NormalizeTypeToken(std::string_view token)
	{
		std::size_t start = 0;
		std::size_t end = token.size();
		while (start < end && std::isspace(static_cast<unsigned char>(token[start])) != 0)
		{
			++start;
		}
		while (end > start && std::isspace(static_cast<unsigned char>(token[end - 1])) != 0)
		{
			--end;
		}
		std::string_view trimmed = token.substr(start, end - start);
		const std::size_t comma = trimmed.find(',');
		if (comma != std::string::npos)
		{
			trimmed = trimmed.substr(0, comma);
		}
		return std::string(trimmed);
	}

	struct BuiltinDescriptor
	{
		Kind kind;
		ManagedKind managedKind;
		const char* managedName;
		const char* cppName;
		bool isValueType;
		bool isPrimitive;
	};

	constexpr BuiltinDescriptor kBuiltinDescriptors[] = {
		{ Kind::Bool,   ManagedKind::System_Boolean, "System.Boolean", "bool",          true,  true },
		{ Kind::Int,    ManagedKind::System_Int32,   "System.Int32",   "int",           true,  true },
		{ Kind::UInt,   ManagedKind::System_UInt32,  "System.UInt32",  "unsigned int",  true,  true },
		{ Kind::Float,  ManagedKind::System_Single,  "System.Single",  "float",         true,  true },
		{ Kind::Double, ManagedKind::System_Double,  "System.Double",  "double",        true,  true },
		{ Kind::String, ManagedKind::System_String,  "System.String",  "std::string",   false, false }
	};
}

MonoTypeResolver::MonoTypeResolver() = default;
MonoTypeResolver::~MonoTypeResolver() = default;

void MonoTypeResolver::Init()
{
	if (m_initialized)
	{
		return;
	}

	m_initialized = true;

	MonoResolver::SetRuntimeResolver(ResolveByType);
	MonoResolver::SetStringResolver(ResolveByName);

	RegisterBuiltinTypes();
}

const MonoTypeDescriptor* MonoTypeResolver::RegisterDescriptor(MonoTypeDescriptor descriptor)
{
	const std::string managedKey = descriptor.managed_name;
	if (!managedKey.empty())
	{
		if (auto it = m_managedLookup.find(managedKey); it != m_managedLookup.end())
		{
			const std::size_t index = it->second;
			auto& stored = m_descriptors[index];

			stored = std::move(descriptor);

			for (auto iter = m_cppLookup.begin(); iter != m_cppLookup.end();)
			{
				if (iter->second == index)
				{
					iter = m_cppLookup.erase(iter);
				}
				else
				{
					++iter;
				}
			}
			if (!stored.cpp_name.empty())
			{
				m_cppLookup.emplace(stored.cpp_name, index);
			}

			for (auto iter = m_hashLookup.begin(); iter != m_hashLookup.end();)
			{
				if (iter->second == index)
				{
					iter = m_hashLookup.erase(iter);
				}
				else
				{
					++iter;
				}
			}
			if (stored.nativeHash != 0)
			{
				m_hashLookup.emplace(stored.nativeHash, index);
			}

			return &stored;
		}
	}

	const std::size_t index = m_descriptors.size();
	m_descriptors.emplace_back(std::move(descriptor));
	auto& stored = m_descriptors.back();

	if (!stored.managed_name.empty())
	{
		m_managedLookup.emplace(stored.managed_name, index);
	}

	if (!stored.cpp_name.empty())
	{
		m_cppLookup.emplace(stored.cpp_name, index);
	}

	if (stored.nativeHash != 0)
	{
		m_hashLookup.emplace(stored.nativeHash, index);
	}

	return &stored;
}

const MonoTypeDescriptor* MonoTypeResolver::FindByManagedName(std::string_view name)
{
	if (name.empty())
	{
		return nullptr;
	}

	std::string key{name};
	auto it = m_managedLookup.find(key);
	if (it != m_managedLookup.end())
	{
		return &m_descriptors[it->second];
	}

	const std::size_t open = key.rfind('[');
	const std::size_t close = (open != std::string::npos) ? key.find(']', open) : std::string::npos;
	if (open != std::string::npos && close != std::string::npos && close > open)
	{
		const std::string_view inside(key.c_str() + open + 1, close - open - 1);
		bool looksLikeArray = inside.empty();
		if (!looksLikeArray)
		{
			looksLikeArray = inside.find_first_not_of(", *") == std::string::npos;
		}

		if (looksLikeArray)
		{
			const std::string elementName = key.substr(0, open);
			const MonoTypeDescriptor* elementDescriptor = FindByManagedName(elementName);
			if (!elementDescriptor)
			{
				elementDescriptor = FindByCppName(elementName);
			}

			if (elementDescriptor)
			{
				return CreateArrayDescriptor(key, elementDescriptor);
			}
		}
	}

	constexpr std::string_view listPrefix = "System.Collections.Generic.List";
	if (key.find(listPrefix) == 0)
	{
		const std::size_t openBracket = key.find('[', listPrefix.length());
		const std::size_t closeBracket = key.rfind(']');
		if (openBracket != std::string::npos && closeBracket != std::string::npos && closeBracket > openBracket)
		{
			std::string elementToken = NormalizeTypeToken(std::string_view(key).substr(openBracket + 1, closeBracket - openBracket - 1));
			if (!elementToken.empty())
			{
				const MonoTypeDescriptor* elementDescriptor = FindByManagedName(elementToken);
				if (!elementDescriptor)
				{
					elementDescriptor = FindByCppName(elementToken);
				}

				if (elementDescriptor)
				{
					return CreateListDescriptor(key, elementDescriptor);
				}
			}
		}
	}

	return nullptr;
}

const MonoTypeDescriptor* MonoTypeResolver::FindByCppName(std::string_view name) const
{
	if (name.empty())
	{
		return nullptr;
	}

	auto it = m_cppLookup.find(std::string{name});
	if (it == m_cppLookup.end())
	{
		return nullptr;
	}

	return &m_descriptors[it->second];
}

const MonoTypeDescriptor* MonoTypeResolver::FindByHash(uint32_t nativeHash) const
{
	if (nativeHash == 0)
	{
		return nullptr;
	}

	auto it = m_hashLookup.find(nativeHash);
	if (it == m_hashLookup.end())
	{
		return nullptr;
	}

	return &m_descriptors[it->second];
}

const MonoTypeDescriptor* MonoTypeResolver::ResolveByType(MonoType* type)
{
	return MonoTypeResolver::Instance().ResolveTypeInternal(type);
}

const MonoTypeDescriptor* MonoTypeResolver::ResolveByName(std::string_view name)
{
	return MonoTypeResolver::Instance().FindByManagedName(name);
}

const MonoTypeDescriptor* MonoTypeResolver::ResolveTypeInternal(MonoType* type)
{
	if (!type)
	{
		return nullptr;
	}

	char* rawName = mono_type_get_name(type);
	if (!rawName)
	{
		return nullptr;
	}

	std::string managedName(rawName);
	mono_free(rawName);

	if (auto* descriptor = FindByManagedName(managedName))
	{
		return descriptor;
	}

	// Handle array types by looking up the element descriptor using the managed-name suffix.
	MonoTypeEnum monoKind = static_cast<MonoTypeEnum>(mono_type_get_type(type));
	if (monoKind == MONO_TYPE_ARRAY || monoKind == MONO_TYPE_SZARRAY)
	{
		MonoClass* arrayClass = mono_class_from_mono_type(type);
		if (arrayClass)
		{
			MonoClass* elementClass = mono_class_get_element_class(arrayClass);
			if (elementClass)
			{
				MonoType* elementType = mono_class_get_type(elementClass);
				if (elementType)
				{
					const MonoTypeDescriptor* elementDescriptor = ResolveTypeInternal(elementType);
					if (!elementDescriptor)
					{
						char* elementNameRaw = mono_type_get_name(elementType);
						if (elementNameRaw)
						{
							std::string elementManagedName(elementNameRaw);
							mono_free(elementNameRaw);
							elementDescriptor = FindByManagedName(elementManagedName);
							if (!elementDescriptor)
							{
								elementDescriptor = FindByCppName(elementManagedName);
							}
						}
					}

					if (!elementDescriptor)
					{
						return nullptr;
					}

					return CreateArrayDescriptor(managedName, elementDescriptor);
				}
			}
		}
	}
	return nullptr;
}

void MonoTypeResolver::RegisterBuiltinTypes()
{
	for (const auto& builtin : kBuiltinDescriptors)
	{
		MonoTypeDescriptor descriptor{};
		descriptor.kind = builtin.kind;
		descriptor.managedKind = builtin.managedKind;
		descriptor.managed_name = builtin.managedName;
		descriptor.cpp_name = builtin.cppName;
		descriptor.isValueType = builtin.isValueType;
		descriptor.isPrimitive = builtin.isPrimitive;
		descriptor.isSerializable = builtin.isPrimitive || builtin.kind == Kind::String;
		descriptor.isPublic = true;

		RegisterDescriptor(std::move(descriptor));
	}
}

const MonoTypeDescriptor* MonoTypeResolver::CreateArrayDescriptor(std::string_view managedName, const MonoTypeDescriptor* elementDescriptor)
{
	if (managedName.empty() || !elementDescriptor)
	{
		return nullptr;
	}

	std::string key{ managedName };
	if (auto it = m_managedLookup.find(key); it != m_managedLookup.end())
	{
		return &m_descriptors[it->second];
	}

	MonoTypeDescriptor descriptor{};
	descriptor.kind = Kind::Array;
	descriptor.managedKind = ManagedKind::System_Array;
	descriptor.managed_name = key;
	if (!elementDescriptor->cpp_name.empty())
	{
		descriptor.cpp_name = elementDescriptor->cpp_name + "[]";
	}
	else
	{
		descriptor.cpp_name = descriptor.managed_name;
	}
	descriptor.isArray = true;
	descriptor.isSerializable = elementDescriptor->isSerializable;
	descriptor.isEngineType = elementDescriptor->isEngineType;
	descriptor.isPrimitive = false;
	descriptor.isValueType = false;
	descriptor.isUserType = elementDescriptor->isUserType;
	descriptor.isPublic = elementDescriptor->isPublic;
	descriptor.elementDescriptor = elementDescriptor;

	return RegisterDescriptor(std::move(descriptor));
}

const MonoTypeDescriptor* MonoTypeResolver::CreateListDescriptor(std::string_view managedName, const MonoTypeDescriptor* elementDescriptor)
{
	if (managedName.empty() || !elementDescriptor)
	{
		return nullptr;
	}

	std::string key{ managedName };
	if (auto it = m_managedLookup.find(key); it != m_managedLookup.end())
	{
		return &m_descriptors[it->second];
	}

	MonoTypeDescriptor descriptor{};
	descriptor.kind = Kind::VectorObject;
	descriptor.managedKind = ManagedKind::System_List;
	descriptor.managed_name = key;
	if (!elementDescriptor->cpp_name.empty())
	{
		descriptor.cpp_name = "std::vector<" + elementDescriptor->cpp_name + ">";
	}
	else
	{
		descriptor.cpp_name = descriptor.managed_name;
	}
	descriptor.isArray = false;
	descriptor.isGeneric = true;
	descriptor.isSerializable = elementDescriptor->isSerializable;
	descriptor.isEngineType = elementDescriptor->isEngineType;
	descriptor.isPrimitive = false;
	descriptor.isValueType = false;
	descriptor.isUserType = elementDescriptor->isUserType;
	descriptor.isPublic = elementDescriptor->isPublic;
	descriptor.elementDescriptor = elementDescriptor;

	return RegisterDescriptor(std::move(descriptor));
}
