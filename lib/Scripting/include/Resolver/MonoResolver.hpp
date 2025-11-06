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