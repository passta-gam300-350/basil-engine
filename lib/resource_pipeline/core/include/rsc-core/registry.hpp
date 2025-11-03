#ifndef RP_RSC_CORE_REGISTRY_HPP
#define RP_RSC_CORE_REGISTRY_HPP

#include <memory>
#include <cassert>
#include <optional>
#include <unordered_map>
#include "rsc-core/loader.hpp"
#include "rsc-core/serialization/serializer.hpp"

namespace rp {
	struct ResourceTypeInfo {
		std::string m_rsc_name;
		std::uint64_t m_rsc_idx;
	};

	//handles management and registration of resource types
	struct ResourceTypeRegistry{
	private:
		ResourceTypeRegistry() = default;
		static constexpr std::uint64_t unbound_type_index{}; //this is technically UB but probability of hash being all 0 is so low that its unlikely to have a collision

		std::unordered_map<std::uint64_t, ResourceTypeInfo> m_rsc_type_info;
		std::unordered_map<std::string, std::uint64_t> m_rsc_typename;

		static std::unique_ptr<ResourceTypeRegistry>& InstancePtr() {
			static std::unique_ptr<ResourceTypeRegistry> s_inst_ptr{new ResourceTypeRegistry()};
			return s_inst_ptr;
		}

		static ResourceTypeRegistry& Instance() {
			return *InstancePtr();
		}

	public:
		static std::optional<std::string> GetResourceTypeName(BasicIndexedGuid& biguid) {
			auto& inst{ Instance() };
			auto it{ inst.m_rsc_type_info.find(biguid.m_typeindex) };
			return it != inst.m_rsc_type_info.end() ? std::make_optional<std::string>(it->second.m_rsc_name) : std::nullopt;
		}
		template <std::uint64_t type_index>
		static void RegisterResourceTypeLoader(ResourceTypeLoader<type_index>, std::string_view type_name_sv) {
			auto& inst{ Instance() };
			std::string type_name{ type_name_sv.begin(), type_name_sv.end() };
			auto it{ inst.m_rsc_typename.find(type_name) };
			assert((it == inst.m_rsc_typename.end() || it->second == unbound_type_index) && "duplicate registration of typename");
			inst.m_rsc_type_info.try_emplace(type_index, ResourceTypeInfo{ type_name, type_index });
			inst.m_rsc_typename.try_emplace(type_name, type_index);
		}
		static void RegisterResourceTypenameForward(std::string_view type_name_sv) {
			auto& inst{ Instance() };
			std::string type_name{ type_name_sv.begin(), type_name_sv.end() };
			auto it{ inst.m_rsc_typename.find(type_name) };
			inst.m_rsc_typename.try_emplace(type_name, unbound_type_index);
		}
	};
}

//put header guards if this is declared in header
//do not register resource type to multiple typename
//no rebinds of resource type allowed
#define RegisterResourceType(TYPE, TYPENAME, NATIVE, LOADER, UNLOADER)						\
template <> struct rp::ResourceTypeLoader<rp::utility::string_hash(TYPENAME)>{				\
	using type = TYPE;																		\
	static constexpr auto type_hash{ rp::utility::string_hash(TYPENAME) };					\
	inline static type Load(std::byte const* data) {										\
		return LOADER(rp::serialization::binary_serializer::deserialize<NATIVE>(data));		\
	}																						\
	inline static void Unload(type&& data) {												\
		return UNLOADER(std::forward<type>(data));											\
	}																						\
};																							\
template <> struct rp::TypeNameGuid<TYPENAME> : public rp::BasicIndexedGuid{				\
	using type = TYPE;																		\
	static constexpr auto type_index{ rp::utility::string_hash(TYPENAME) };					\
	inline static auto registration{[]{														\
			rp::ResourceTypeRegistry::RegisterResourceTypeLoader(rp::ResourceTypeLoader<rp::utility::string_hash(TYPENAME)>(), TYPENAME);	\
			return rp::utility::type_hash<TYPE>::value();									\
			}() };																			\
	TypeNameGuid() : BasicIndexedGuid{rp::null_guid, type_index}{}							\
	static std::string_view GetTypeName(){													\
		return TYPENAME;																	\
	}																						\
};																							\
template <> struct rp::TypedGuid<TYPE> : public rp::BasicIndexedGuid{						\
	using type = TYPE;																		\
	static constexpr auto type_index{rp::utility::string_hash(TYPENAME)};					\
	TypedGuid() : BasicIndexedGuid{rp::null_guid, type_index}{}								\
	static std::string_view GetTypeName(){													\
		return TYPENAME;																	\
	}																						\
};																				

//put header guards if this is declared in header
//do not register resource type to multiple typename
//no rebinds of resource type allowed
//forward registers the type, does not define loader and unloader
//do not mix with RegisterResourceType. this is for when the definition of the loader is defined externally
#define RegisterResourceTypeForward(TYPE, TYPENAME, DEFINE_LOADER_ALIASE)					\
template <> struct rp::ResourceTypeLoader<rp::utility::string_hash(TYPENAME)>{				\
	using type = TYPE;																		\
	static constexpr auto type_hash{ rp::utility::string_hash(TYPENAME) };					\
	inline static rp::ResourceTypeLoaderData<type> GetLoaderData() {						\
		return rp::ResourceTypeLoaderData<type>(Load, Unload);								\
	}																						\
	static type Load(std::byte const* data);												\
	static void Unload(type&& data);														\
};																							\
template <> struct rp::TypeNameGuid<TYPENAME> : public rp::BasicIndexedGuid{				\
	using type = TYPE;																		\
	static constexpr auto type_index{ rp::utility::string_hash(TYPENAME) };					\
	inline static auto registration{[]{														\
			rp::ResourceTypeRegistry::RegisterResourceTypeLoader(rp::ResourceTypeLoader<rp::utility::string_hash(TYPENAME)>(), TYPENAME);	\
			return rp::utility::type_hash<TYPE>::value();									\
			}() };																			\
	TypeNameGuid() : BasicIndexedGuid{rp::null_guid, type_index}{}							\
	static std::string_view GetTypeName(){													\
		return TYPENAME;																	\
	}																						\
};																							\
template <> struct rp::TypedGuid<TYPE> : public rp::BasicIndexedGuid{						\
	using type = TYPE;																		\
	static constexpr auto type_index{rp::utility::string_hash(TYPENAME)};					\
	TypedGuid() : BasicIndexedGuid{rp::null_guid, type_index}{}								\
	static std::string_view GetTypeName(){													\
		return TYPENAME;																	\
	}																						\
};																							\
using DEFINE_LOADER_ALIASE = rp::ResourceTypeLoader<rp::utility::string_hash(TYPENAME)>;

//put header guards if this is declared in header
//do not register resource type to multiple typename
//forward registers the type, does not define loader and unloader
//do not mix with RegisterResourceType. this is for when the resource type is not known or no forward declaration
//Note: do not put quotes in the parameter this is not a string!
//Note: you must call bind in the defining source file
#define RegisterResourceTypenameForward(TYPENAME)											\
inline static auto TYPENAME##_forward_registration{ [] {									\
			rp::ResourceTypeRegistry::RegisterResourceTypenameForward(#TYPENAME);			\
			return 0;																		\
			}() };

#endif