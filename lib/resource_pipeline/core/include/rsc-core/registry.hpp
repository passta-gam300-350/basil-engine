#ifndef RP_RSC_CORE_REGISTRY_HPP
#define RP_RSC_CORE_REGISTRY_HPP

#include <memory>
#include <optional>
#include <unordered_map>
#include "rsc-core/loader.hpp"

namespace rp {
	struct ResourceTypeInfo {
		std::string m_rsc_name;
		std::uint64_t m_rsc_idx;
	};

	//handles management and registration of resource types (compile time)
	struct ResourceTypeRegistry{
	private:
		ResourceTypeRegistry() = default;

		std::unordered_map<std::uint64_t, ResourceTypeInfo> m_rsc_type_info;
		std::unordered_map<std::string, std::uint64_t> m_rsc_typename;

		std::unique_ptr<ResourceTypeRegistry>& InstancePtr() {
			static std::unique_ptr<ResourceTypeRegistry> s_inst_ptr{std::make_unique<ResourceTypeRegistry>()};
			return s_inst_ptr;
		}

		ResourceTypeRegistry& Instance() {
			return *InstancePtr();
		}

	public:
		std::optional<std::string> GetResourceTypeName(BasicIndexedGuid biguid) {
			auto& inst{ Instance() };
			auto it{ inst.m_rsc_type_info.find(biguid.m_typeindex) };
			return it != inst.m_rsc_type_info.begin() ? std::make_optional<std::string>(it->second.m_rsc_name) : std::nullopt;
		}
	};
}

//put header guards if this is declared in header
//do not register resource type to multiple typename
//no rebinds of resource type allowed
#define RegisterResourceType(TYPE, TYPENAME, LOADER, UNLOADER)								\
template <> struct rp::ResourceTypeLoader<rp::utility::type_hash<TYPE>::value()>{			\
	using type = TYPE;																		\
	inline static const rp::ResourceTypeLoaderData<type> sc_loader_data{LOADER, UNLOADER};	\
	static constexpr auto type_hash{ rp::utility::type_hash<TYPE>::value() };				\
	inline static rp::ResourceTypeLoaderData<type> GetLoaderData() {						\
		return sc_loader_data;																\
	}																						\
	inline static type Load(std::byte const* data) {										\
		return GetLoaderData().m_loader(data);												\
	}																						\
	inline static void Unload(type&& data) {												\
		return GetLoaderData().m_unloader(std::forward<type>(data));						\
	}																						\
};																							\
template <> struct rp::TypeNameGuid<TYPENAME> : public rp::BasicIndexedGuid{				\
	using type = TYPE;																		\
	static constexpr auto type_index{ rp::utility::type_hash<TYPE>::value() };				\
	TypeNameGuid() : BasicIndexedGuid{rp::null_guid, type_index}{}							\
};

#endif