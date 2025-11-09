/******************************************************************************/
/*!
\file   MonoReflectionRegistry.cpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the implementation for the MonoReflectionRegistry class, which
provides an interface for managing reflection data for managed types in the engine.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Manager/MonoReflectionRegistry.hpp"
#include "Manager/MonoTypeResolver.hpp"
#include "MonoResolver/MonoTypeDescriptor.hpp"

#include <string>
#include <utility>

AssemblyNode* MonoReflectionTree::GetOrAddAssembly(std::string_view assemblyName)
{
	std::string key{ assemblyName };
	auto it = m_assemblies.find(key);
	if (it != m_assemblies.end())
	{
		return it->second.get();
	}

	auto node = std::make_unique<AssemblyNode>(key);
	AssemblyNode* raw = node.get();
	m_assemblies.emplace(std::move(key), std::move(node));
	return raw;
}

ClassNode* MonoReflectionTree::GetOrAddClass(std::string_view assemblyName, std::string_view className)
{
	AssemblyNode* assemblyNode = GetOrAddAssembly(assemblyName);
	if (!assemblyNode)
	{
		return nullptr;
	}

	std::string key{ className };
	auto& classes = assemblyNode->classes;
	auto it = classes.find(key);
	if (it != classes.end())
	{
		ClassNode* existing = it->second.get();
		if (!existing->descriptor)
		{
			const MonoTypeDescriptor* descriptor = MonoTypeResolver::Instance().FindByManagedName(existing->name);
			if (!descriptor)
			{
				descriptor = MonoTypeResolver::Instance().FindByCppName(existing->name);
			}
			existing->descriptor = descriptor;
		}
		return existing;
	}

	auto node = std::make_unique<ClassNode>(key, assemblyNode);
	ClassNode* raw = node.get();

	const MonoTypeDescriptor* descriptor = MonoTypeResolver::Instance().FindByManagedName(key);
	if (!descriptor)
	{
		descriptor = MonoTypeResolver::Instance().FindByCppName(key);
	}
	raw->descriptor = descriptor;

	classes.emplace(std::move(key), std::move(node));
	return raw;
}

FieldNode* MonoReflectionTree::RegisterField(std::string_view assemblyName, std::string_view className, std::string_view fieldName, std::string_view typeName, bool isStatic, bool isPublic)
{
	ClassNode* classNode = GetOrAddClass(assemblyName, className);
	if (!classNode)
	{
		return nullptr;
	}

	std::string key{ fieldName };
	auto& fields = classNode->fields;
	auto it = fields.find(key);
	if (it != fields.end())
	{
	FieldNode* existing = it->second.get();
	existing->type = std::string(typeName);
	existing->isStatic = isStatic;
	existing->isPublic = isPublic;
		const MonoTypeDescriptor* descriptor = MonoTypeResolver::Instance().FindByManagedName(existing->type);
		if (!descriptor)
		{
			descriptor = MonoTypeResolver::Instance().FindByCppName(existing->type);
		}
		existing->descriptor = descriptor;
		return existing;
	}

	auto fieldNode = std::make_unique<FieldNode>(key, std::string(typeName), isStatic, isPublic, classNode);
	fieldNode->descriptor = MonoTypeResolver::Instance().FindByManagedName(fieldNode->type);
	if (!fieldNode->descriptor)
	{
		fieldNode->descriptor = MonoTypeResolver::Instance().FindByCppName(fieldNode->type);
	}
	FieldNode* raw = fieldNode.get();
	fields.emplace(std::move(key), std::move(fieldNode));
	return raw;
}

const AssemblyNode* MonoReflectionTree::FindAssembly(std::string_view assemblyName) const
{
	std::string key{ assemblyName };
	auto it = m_assemblies.find(key);
	return it != m_assemblies.end() ? it->second.get() : nullptr;
}

const ClassNode* MonoReflectionTree::FindClass(std::string_view assemblyName, std::string_view className) const
{
	const AssemblyNode* assemblyNode = FindAssembly(assemblyName);
	if (!assemblyNode)
	{
		return nullptr;
	}

	std::string key{ className };
	auto it = assemblyNode->classes.find(key);
	return it != assemblyNode->classes.end() ? it->second.get() : nullptr;
}

void MonoReflectionTree::Clear()
{
	m_assemblies.clear();
}

void MonoReflectionTree::ForEachAssembly(const std::function<void(const AssemblyNode&)>& visitor) const
{
	if (!visitor)
	{
		return;
	}

	for (const auto& [_, node] : m_assemblies)
	{
		if (node)
		{
			visitor(*node);
		}
	}
}

ClassNode* MonoReflectionTree::FindClassByManagedName(std::string_view className)
{
	if (className.empty())
	{
		return nullptr;
	}

	std::string key{ className };
	for (auto& [_, assemblyNode] : m_assemblies)
	{
		if (!assemblyNode)
		{
			continue;
		}

		auto it = assemblyNode->classes.find(key);
		if (it != assemblyNode->classes.end() && it->second)
		{
			return it->second.get();
		}
	}

	return nullptr;
}

const ClassNode* MonoReflectionTree::FindClassByManagedName(std::string_view className) const
{
	if (className.empty())
	{
		return nullptr;
	}

	std::string key{ className };
	for (const auto& [_, assemblyNode] : m_assemblies)
	{
		if (!assemblyNode)
		{
			continue;
		}

		auto it = assemblyNode->classes.find(key);
		if (it != assemblyNode->classes.end() && it->second)
		{
			return it->second.get();
		}
	}

	return nullptr;
}

MonoReflectionRegistry& MonoReflectionRegistry::Instance()
{
	static MonoReflectionRegistry instance;
	return instance;
}

AssemblyNode* MonoReflectionRegistry::GetOrAddAssembly(std::string_view assemblyName)
{
	return m_tree.GetOrAddAssembly(assemblyName);
}

ClassNode* MonoReflectionRegistry::GetOrAddClass(std::string_view assemblyName, std::string_view className)
{
	return m_tree.GetOrAddClass(assemblyName, className);
}

FieldNode* MonoReflectionRegistry::RegisterField(std::string_view assemblyName, std::string_view className, std::string_view fieldName, std::string_view typeName, bool isStatic, bool isPublic)
{
    return m_tree.RegisterField(assemblyName, className, fieldName, typeName, isStatic, isPublic);
}

const AssemblyNode* MonoReflectionRegistry::FindAssembly(std::string_view assemblyName) const
{
	return m_tree.FindAssembly(assemblyName);
}

const ClassNode* MonoReflectionRegistry::FindClass(std::string_view assemblyName, std::string_view className) const
{
	return m_tree.FindClass(assemblyName, className);
}

void MonoReflectionRegistry::Clear()
{
	m_tree.Clear();
}

void MonoReflectionRegistry::VisitAssemblies(const std::function<void(const AssemblyNode&)>& visitor) const
{
	m_tree.ForEachAssembly(visitor);
}

ClassNode* MonoReflectionRegistry::FindClassByManagedName(std::string_view className)
{
	return m_tree.FindClassByManagedName(className);
}

const ClassNode* MonoReflectionRegistry::FindClassByManagedName(std::string_view className) const
{
	return m_tree.FindClassByManagedName(className);
}
