/******************************************************************************/
/*!
\file   MonoResolver.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the declaration for the MonoResolver class, which
provides an interface for resolving managed types and their members in the Mono runtime.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef MONO_RESOLVER_HPP
#define MONO_RESOLVER_HPP
#include <functional>
#include <string_view>
struct MonoTypeDescriptor;
typedef struct _MonoType  MonoType;


using RuntimeResolver = std::function<const MonoTypeDescriptor* (MonoType*)>;
using StringResolver = std::function<const MonoTypeDescriptor* (std::string_view)>;

class MonoResolver {
	static RuntimeResolver m_runtimeResolver;
	static StringResolver m_stringResolver;
	public:
		static void SetRuntimeResolver(RuntimeResolver const& resolver);
		static void SetStringResolver(StringResolver const& resolver);
		static const MonoTypeDescriptor* ResolveType(MonoType* type);
		static const MonoTypeDescriptor* ResolveType(std::string_view name);
};

#endif // MONO_RESOLVER_HPP