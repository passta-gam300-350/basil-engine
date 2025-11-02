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

