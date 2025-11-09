/******************************************************************************/
/*!
\file   MonoResolver.cpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the implementation for the MonoResolver class, which
provides an interface for resolving managed types and their members in the Mono runtime.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Resolver/MonoResolver.hpp"

RuntimeResolver MonoResolver::m_runtimeResolver = nullptr;
StringResolver MonoResolver::m_stringResolver = nullptr;

void MonoResolver::SetRuntimeResolver(RuntimeResolver const& resolver) {
	m_runtimeResolver = resolver;
}

void MonoResolver::SetStringResolver(StringResolver const& resolver) {
	m_stringResolver = resolver;
}

const MonoTypeDescriptor* MonoResolver::ResolveType(MonoType* type) {
	if (m_runtimeResolver) {
		return m_runtimeResolver(type);
	}
	return nullptr;
}
const MonoTypeDescriptor* MonoResolver::ResolveType(std::string_view name) {
	if (m_stringResolver) {
		return m_stringResolver(name);
	}
	return nullptr;
}

