/******************************************************************************/
/*!
\file   MonoReflectionRegistry.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the declaration for the MonoReflectionRegistry class, which
provides an interface for managing reflection data for managed types in the engine.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef MONO_REFLECTION_REGISTRY_HPP
#define MONO_REFLECTION_REGISTRY_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

struct CSAssembly;
struct MonoTypeDescriptor;

enum class ReflectionKind : uint8_t
{
	Assembly,
	Class,
	Field
};

struct ReflectionNode
{
	explicit ReflectionNode(std::string name, ReflectionKind kind, ReflectionNode* parent = nullptr) :
		name(std::move(name)), kind(kind), parent(parent) {}
	virtual ~ReflectionNode() = default;

	std::string name;
	ReflectionKind kind;
	ReflectionNode* parent = nullptr;
};

struct ClassNode;

struct FieldNode : ReflectionNode
{
	FieldNode(std::string name, std::string typeName = {}, bool isStatic = false, bool isPublic = false, ReflectionNode* parent = nullptr) :
		ReflectionNode(std::move(name), ReflectionKind::Field, parent),
		type(std::move(typeName)), isStatic(isStatic), isPublic(isPublic) {}

	std::string type;
	bool isStatic = false;
	const MonoTypeDescriptor* descriptor = nullptr;
	bool isPublic = false;
};

struct ClassNode : ReflectionNode
{
	using FieldMap = std::unordered_map<std::string, std::unique_ptr<FieldNode>>;

	explicit ClassNode(std::string name, ReflectionNode* parent = nullptr) :
		ReflectionNode(std::move(name), ReflectionKind::Class, parent) {}

	FieldMap fields;
	const MonoTypeDescriptor* descriptor = nullptr;
};

struct AssemblyNode;

class MonoReflectionTree
{
public:
	MonoReflectionTree() = default;

	AssemblyNode* GetOrAddAssembly(std::string_view assemblyName);
	ClassNode* GetOrAddClass(std::string_view assemblyName, std::string_view className);
	FieldNode* RegisterField(std::string_view assemblyName, std::string_view className, std::string_view fieldName, std::string_view typeName, bool isStatic, bool isPublic);

	const AssemblyNode* FindAssembly(std::string_view assemblyName) const;
	const ClassNode* FindClass(std::string_view assemblyName, std::string_view className) const;
	ClassNode* FindClassByManagedName(std::string_view className);
	const ClassNode* FindClassByManagedName(std::string_view className) const;

	void Clear();
	void ForEachAssembly(const std::function<void(const AssemblyNode&)>& visitor) const;

private:
	std::unordered_map<std::string, std::unique_ptr<AssemblyNode>> m_assemblies;
};

struct AssemblyNode : ReflectionNode
{
	using ClassMap = std::unordered_map<std::string, std::unique_ptr<ClassNode>>;

	explicit AssemblyNode(std::string name) :
		ReflectionNode(std::move(name), ReflectionKind::Assembly, nullptr) {}

	ClassMap classes;
};

class MonoReflectionRegistry
{
public:
	static MonoReflectionRegistry& Instance();

	AssemblyNode* GetOrAddAssembly(std::string_view assemblyName);
	ClassNode* GetOrAddClass(std::string_view assemblyName, std::string_view className);
	FieldNode* RegisterField(std::string_view assemblyName, std::string_view className, std::string_view fieldName, std::string_view typeName, bool isStatic, bool isPublic);

	const AssemblyNode* FindAssembly(std::string_view assemblyName) const;
	const ClassNode* FindClass(std::string_view assemblyName, std::string_view className) const;
    ClassNode* FindClassByManagedName(std::string_view className);
    const ClassNode* FindClassByManagedName(std::string_view className) const;

	void Clear();
	void VisitAssemblies(const std::function<void(const AssemblyNode&)>& visitor) const;

private:
	MonoReflectionRegistry() = default;

	MonoReflectionTree m_tree;
};


#endif // MONO_REFLECTION_REGISTRY_HPP
