/******************************************************************************/
/*!
\file   MonoTypeResolver.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the declaration for the MonoTypeResolver class, which
is responsible for resolving Mono type information based on type names and mono
type descriptors.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef MONO_TYPE_RESOLVER_HPP
#define MONO_TYPE_RESOLVER_HPP
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <deque>
#include "MonoResolver/MonoTypeDescriptor.hpp"
typedef struct _MonoType  MonoType;
class MonoTypeResolver
{
	static const MonoTypeDescriptor* ResolveByType(MonoType* type);
	static const MonoTypeDescriptor* ResolveByName(std::string_view name);

	const MonoTypeDescriptor* ResolveTypeInternal(MonoType* type);
	const MonoTypeDescriptor* CreateArrayDescriptor(std::string_view managedName, const MonoTypeDescriptor* elementDescriptor);
	const MonoTypeDescriptor* CreateListDescriptor(std::string_view managedName, const MonoTypeDescriptor* elementDescriptor);
	void RegisterBuiltinTypes();

	bool m_initialized = false;
	std::deque<MonoTypeDescriptor> m_descriptors;
	std::unordered_map<std::string, std::size_t> m_managedLookup;
	std::unordered_map<std::string, std::size_t> m_cppLookup;
	std::unordered_map<uint32_t, std::size_t> m_hashLookup;

public:



	static MonoTypeResolver& Instance()
	{
		static MonoTypeResolver instance;
		instance.Init();
		return instance;
	}

	void Init();

	const MonoTypeDescriptor* RegisterDescriptor(MonoTypeDescriptor descriptor);
	const MonoTypeDescriptor* FindByManagedName(std::string_view name);
	const MonoTypeDescriptor* FindByCppName(std::string_view name) const;
	const MonoTypeDescriptor* FindByHash(uint32_t nativeHash) const;

	MonoTypeResolver();
	~MonoTypeResolver();
	// Add methods and members as needed for type resolution
};

#endif // MONO_TYPE_RESOLVER_HPP
