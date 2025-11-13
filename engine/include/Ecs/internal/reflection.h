/******************************************************************************/
/*!
\file   reflection.h
\author Team PASSTA
		Chew Bangxin Steven (banxginsteven.chew@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/09
\brief    Declares the reflection registry and the reflection registration macros

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef LIB_REFLECTION_REGISTRATION_H
#define LIB_REFLECTION_REGISTRATION_H
#include <rsc-core/rp.hpp>
#include <entt/entt.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/container.hpp>
#include <entt/core/hashed_string.hpp>
#include <vector>
#include <functional>
#include <iostream>
#include <fstream>
using TypeName = entt::hashed_string;
constexpr TypeName ToTypeName(std::string_view name) { return entt::hashed_string(name.data(), name.length()); };

constexpr TypeName operator""_tn(const char* str, std::size_t len) noexcept {
	return entt::hashed_string(str, len);
}
/*
struct TypeInfo {
	entt::meta_type m_MetaType;
};*/

using TypeInfo = entt::meta_type;

struct TypeData {
	void* m_Raw;
	entt::meta_type m_TypeInfo;

	TypeData& SetRaw(void*);
	std::string TypeName();
	entt::id_type TypeId();
	bool IsPrimitive();
	auto begin() { //lowercase for compatibility with std::ranges
		entt::meta_any any = m_TypeInfo.from_void(m_Raw);
		return any.type().data().begin();
	}
	auto end(); //lowercase for compatibility with std::ranges
};

TypeInfo ResolveType(TypeName t_name);

template<std::size_t N>
struct static_string {
	char data[N];
	constexpr static_string(const char(&str)[N]) {
		for (std::size_t i = 0; i < N; ++i) data[i] = str[i];
	}
};

template<static_string S>
constexpr auto operator"" _rs() { return S; }

struct ComponentBinarySerializer {
	std::function<void(entt::snapshot&, std::ostream&)> m_Saver;
	std::function<void(entt::snapshot_loader&, std::istream&)> m_Loader;
};

struct ReflectionRegistry {
	using TypeID = entt::id_type; // same as std::uint32_t

	struct Detail {
		std::unordered_map<TypeID, entt::meta_type> m_Storage;
		std::unordered_map<TypeID, TypeID> m_InternalTypeid;
		std::unordered_map<TypeID, std::string> m_Names;
		std::unordered_map<TypeID, std::unordered_map<TypeID, std::string>> m_FieldNames;
		std::vector<ComponentBinarySerializer> m_BinRegistry;
	};

	static ReflectionRegistry::Detail& Registry() {
		static ReflectionRegistry::Detail instance;
		return instance;
	}

	static std::unordered_map<TypeID, entt::meta_type>& types() {
		return Registry().m_Storage;
	}

	static std::unordered_map<TypeID, TypeID>& InternalID() {
		return Registry().m_InternalTypeid;
	}

	static std::unordered_map<TypeID, std::string>& TypeNames() {
		return Registry().m_Names;
	}

	static std::unordered_map<TypeID, std::string>& RegisterType(TypeID type_id, std::string_view name) {
		ReflectionRegistry::Detail& reg{ Registry() };
		reg.m_Names.try_emplace(type_id, name);
		reg.m_FieldNames.try_emplace(type_id, std::unordered_map<TypeID, std::string>{});
		return reg.m_FieldNames[type_id];
	}

	static std::unordered_map<TypeID, std::string>& GetFieldNames(TypeID type_id) {
		ReflectionRegistry::Detail& reg{ Registry() };
		assert(reg.m_FieldNames.find(type_id) != reg.m_FieldNames.end());
		return reg.m_FieldNames[type_id];
	}

	static std::string const& GetTypeName(TypeID type_id) {
		ReflectionRegistry::Detail& reg{ Registry() };
		assert(reg.m_Names.find(type_id) != reg.m_Names.end());
		return reg.m_Names[type_id];
	}

	// Helper to get TypeID from a type (wrapper around entt::type_hash)
	template<typename T>
	static constexpr TypeID GetTypeID() {
		return entt::type_hash<T>::value();
	}

	static std::vector<ComponentBinarySerializer>& BinSerializerRegistryInstance() {
		return Registry().m_BinRegistry;
	}

	// do not use unless u know what you are doing. resets the whole reflection registry. this means you will lose all registered reflection data leading to unexpected behaviors. here mainly for testing and other debugging purposes
	static void Reset() {
		ReflectionRegistry::Detail& reg{ Registry() };
		reg.m_Storage.clear();
		reg.m_Names.clear();
		reg.m_FieldNames.clear();
		reg.m_BinRegistry.clear();
		entt::meta_reset();
	}

	// registers native types, call once
	static void SetupNativeTypes();

	static void SetupEngineTypes();
};

//templated serialiser, Node must overload[]
//serialise type recursively using reflection (impt: user defined type and user defined nested member types must be registered to work properly)
template <typename Node>
void SerializeType(const entt::meta_any& obj, Node& out) {
	auto type = obj.type();
	auto field_table = ReflectionRegistry::GetFieldNames(type.id());

	auto a = ReflectionRegistry::Registry();

	for (auto [id, data] : type.data()) {
		std::string field_name{ field_table[id] }; 

		auto value = data.get(obj);
		auto meta_type = value.type(); 
		if (meta_type.data().begin() != meta_type.data().end()) {
			Node nested;
			SerializeType(value, nested);
			out[field_name] = nested;
		}
		else {

			if (meta_type.is_enum())
			{
				const void* val_ptr = value.base().data();
				if (meta_type.size_of() == sizeof(int))
				{
					int enum_value = *static_cast<const int*>(val_ptr);
					out[field_name] = enum_value;
				}
				else if (meta_type.size_of() == sizeof(unsigned int))
				{
					unsigned int enum_value = *static_cast<const unsigned int*>(val_ptr);
					out[field_name] = enum_value;
				}
				else if (meta_type.size_of() == sizeof(short))
				{
					short enum_value = *static_cast<const short*>(val_ptr);
					out[field_name] = enum_value;
				}
				else if (meta_type.size_of() == sizeof(unsigned short))
				{
					unsigned short enum_value = *static_cast<const unsigned short*>(val_ptr);
					out[field_name] = enum_value;
				}
				else if (meta_type.size_of() == sizeof(char))
				{
					char enum_value = *static_cast<const char*>(val_ptr);
					out[field_name] = enum_value;
				}
				else if (meta_type.size_of() == sizeof(unsigned char))
				{
					unsigned char enum_value = *static_cast<const unsigned char*>(val_ptr);
					out[field_name] = enum_value;
				}
				else if (meta_type.size_of() == sizeof(long))
				{
					long enum_value = *static_cast<const long*>(val_ptr);
					out[field_name] = enum_value;
				}
				else if (meta_type.size_of() == sizeof(unsigned long))
				{
					unsigned long enum_value = *static_cast<const unsigned long*>(val_ptr);
					out[field_name] = enum_value;
				}
				else if (meta_type.size_of() == sizeof(long long))
				{
					long long enum_value = *static_cast<const long long*>(val_ptr);
					out[field_name] = enum_value;
				}
				else if (meta_type.size_of() == sizeof(unsigned long long))
				{
					unsigned long long enum_value = *static_cast<const unsigned long long*>(val_ptr);
					out[field_name] = enum_value;
				}
				else
				{
					std::cerr << "Unsupported enum underlying type size: " << meta_type.size_of() << " bytes\n";
				}
			}
		
			if (std::unordered_map<std::string, rp::BasicIndexedGuid> const* v = value.try_cast<std::unordered_map<std::string, rp::BasicIndexedGuid> const>()) {
				for (auto& [name, guid] : *v) {
					out[field_name][name]["guid"] = guid.m_guid.to_hex();
					out[field_name][name]["type"] = guid.m_typeindex;
				}
			}
			else if (rp::BasicIndexedGuid const* vbig = value.try_cast<rp::BasicIndexedGuid const>()) {
				out[field_name]["guid"] = vbig->m_guid.to_hex();
				out[field_name]["type"] = vbig->m_typeindex;
			}
			else if (rp::Guid const* vg = value.try_cast<rp::Guid const>())
				out[field_name] = vg->to_hex();

			// MaterialOverridesComponent map serialization
			else if (std::unordered_map<std::string, float> const* mapFloat = value.try_cast<std::unordered_map<std::string, float> const>()) {
				for (auto& [key, val] : *mapFloat) {
					out[field_name][key] = val;
				}
			}
			else if (std::unordered_map<std::string, glm::vec3> const* mapVec3 = value.try_cast<std::unordered_map<std::string, glm::vec3> const>()) {
				for (auto& [key, val] : *mapVec3) {
					Node nested;
					auto vec3_any = entt::meta_any{val};
					SerializeType(vec3_any, nested);
					out[field_name][key] = nested;
				}
			}
			else if (std::unordered_map<std::string, glm::vec4> const* mapVec4 = value.try_cast<std::unordered_map<std::string, glm::vec4> const>()) {
				for (auto& [key, val] : *mapVec4) {
					Node nested;
					auto vec4_any = entt::meta_any{val};
					SerializeType(vec4_any, nested);
					out[field_name][key] = nested;
				}
			}
			else if (std::unordered_map<std::string, glm::mat4> const* mapMat4 = value.try_cast<std::unordered_map<std::string, glm::mat4> const>()) {
				for (auto& [key, val] : *mapMat4) {
					Node nested;
					auto mat4_any = entt::meta_any{val};
					SerializeType(mat4_any, nested);
					out[field_name][key] = nested;
				}
			}

			else if (std::vector<std::uint32_t> const* vui = value.try_cast<std::vector<std::uint32_t> const>()) {
				for (auto& vuint : *vui) {
					out[field_name].push_back(vuint);
				}
			}
			else if (std::vector<std::string> const* vstr = value.try_cast<std::vector<std::string> const>()) {
				for (auto const& entry : *vstr) {
					out[field_name].push_back(entry);
				}
			}
			else if (std::vector<rp::Guid> const* vguid = value.try_cast<std::vector<rp::Guid> const>()) {
				for (auto const& guid : *vguid) {
					out[field_name].push_back(guid.to_hex());
				}
			}
			

			// primitives
			else if (int const* vi = value.try_cast<int const>())
				out[field_name] = *vi;
			else if (float const* vf = value.try_cast<float const>())
				out[field_name] = *vf;
			else if (double const* vd = value.try_cast<double const>())
				out[field_name] = *vd;
			else if (uint32_t const* uint32 = value.try_cast<uint32_t const>())
				out[field_name] = *uint32;
			else if (std::string const* vs = value.try_cast<std::string const>())
				out[field_name] = *vs;
			else if (bool const* vb = value.try_cast<bool const>())
				out[field_name] = *vb;
		

		}
	}
}

//templated serialiser, Node must overload[](std::string const&)
template <typename Node>
Node SerializeEntity(entt::registry& reg, entt::entity e) {
	Node entity_node;

	for (auto& [type_id, meta_type] : ReflectionRegistry::types()) {
		auto storage = reg.storage(type_id);
		if (storage && storage->contains(e)) {
			auto comp_any = meta_type.from_void(storage->value(e));
			Node comp_node;
			std::string test = ReflectionRegistry::GetTypeName(meta_type.id());
			std::string val{};
			if (test == "entity name") {
				val = *reinterpret_cast<std::string*>(storage->value(e));
			}
			SerializeType(comp_any, comp_node);
 			entity_node[ReflectionRegistry::GetTypeName(meta_type.id())] = comp_node;
		}
	}

	return entity_node;
}

//templated deserialiser, Node must overload[](std::string const&)
template<typename Node>
void DeserializeType(const Node& in, entt::meta_any& obj) {
	const auto type = obj.type();
	auto field_table = ReflectionRegistry::GetFieldNames(type.id());

	// Iterate over all reflected data members
	for (auto [id, data] : type.data()) {
		std::string field_name{ field_table[id] };

		// Skip if YAML doesn't have this field
		if (!in[field_name]) {
			continue;
		}

		const auto member_type = data.type();
		const auto mid = member_type.id();

		// If the member type itself has reflected members, recurse
		if (member_type.data().begin() != member_type.data().end()) {
			entt::meta_any nested = member_type.construct();
			DeserializeType(in[field_name], nested);
			data.set(obj, nested);
			continue;
		}

		if (member_type.is_enum()) {

			
			if (member_type.size_of() == sizeof(int)) {
				int underlying_value = in[field_name].template as<int>();
				auto enum_any = member_type.from_void(&underlying_value);
				data.set(obj, enum_any);
			}
			else if (member_type.size_of() == sizeof(unsigned int)) {
				unsigned int underlying_value = in[field_name].template as<unsigned int>();
				auto enum_any = member_type.from_void(&underlying_value);
				data.set(obj, enum_any);
			}
			else if (member_type.size_of() == sizeof(short)) {
				short underlying_value = in[field_name].template as<short>();
				auto enum_any = member_type.from_void(&underlying_value);
				data.set(obj, enum_any);
			}
			else if (member_type.size_of() == sizeof(unsigned short)) {
				unsigned short underlying_value = in[field_name].template as<unsigned short>();
				auto enum_any = member_type.from_void(&underlying_value);
				data.set(obj, enum_any);
			}
			else if (member_type.size_of() == sizeof(char)) {
				char underlying_value = in[field_name].template as<char>();
				auto enum_any = member_type.from_void(&underlying_value);
				data.set(obj, enum_any);
			}
			else if (member_type.size_of() == sizeof(unsigned char)) {
				unsigned char underlying_value = in[field_name].template as<unsigned char>();
				auto enum_any = member_type.from_void(&underlying_value);
				data.set(obj, enum_any);
			}
			else if (member_type.size_of() == sizeof(long)) {
				long underlying_value = in[field_name].template as<long>();
				auto enum_any = member_type.from_void(&underlying_value);
				data.set(obj, enum_any);
			}
			else if (member_type.size_of() == sizeof(unsigned long)) {
				unsigned long underlying_value = in[field_name].template as<unsigned long>();
				auto enum_any = member_type.from_void(&underlying_value);
				data.set(obj, enum_any);
			}
			else if (member_type.size_of() == sizeof(long long)) {
				long long underlying_value = in[field_name].template as<long long>();
				auto enum_any = member_type.from_void(&underlying_value);
				data.set(obj, enum_any);
			}
			else if (member_type.size_of() == sizeof(unsigned long long)) {
				unsigned long long underlying_value = in[field_name].template as<unsigned long long>();
				auto enum_any = member_type.from_void(&underlying_value);
				data.set(obj, enum_any);
			}
			else {
				/*std::cerr << "Unsupported enum underlying type size during deserialization: " << member_type.size_of() << " bytes\n";*/
				assert(false && "Unsupported enum underlying type size during deserialization");
			}
			continue;
		}

		if (mid == entt::type_hash<rp::Guid>::value()) {
			std::string guid_str = in[field_name].template as<std::string>();
			rp::Guid guid = rp::Guid::to_guid(guid_str);
			data.set(obj, guid);
			continue;
		}

		if (mid == entt::type_hash<rp::BasicIndexedGuid>::value()) {
			std::string guid_str = in[field_name]["guid"].template as<std::string>();
			rp::BasicIndexedGuid guid{};
			guid.m_guid = rp::Guid::to_guid(guid_str);
			guid.m_typeindex = in[field_name]["type"].template as<std::uint64_t>();
			data.set(obj, guid);
			continue;
		}

		if (mid == entt::type_hash<std::unordered_map<std::string, rp::BasicIndexedGuid>>::value()) {
			auto mapnode = in[field_name];
			std::unordered_map<std::string, rp::BasicIndexedGuid> map_name_guid{};
			if (mapnode.IsMap() && mapnode.begin()->first.template as<std::string>() != "guid") {
				for (auto const& pair : mapnode) {
					rp::BasicIndexedGuid biguid{};
					biguid.m_guid = rp::Guid::to_guid(pair.second["guid"].template as<std::string>());
					biguid.m_typeindex = pair.second["type"].template as<std::uint64_t>();
					map_name_guid[pair.first.template as<std::string>()] = biguid;
				}
			}
			else {
				rp::BasicIndexedGuid biguid{};
				biguid.m_guid = rp::Guid::to_guid(mapnode["guid"].template as<std::string>());
				biguid.m_typeindex = mapnode["type"].template as<std::uint64_t>();
				map_name_guid["unnamed slot"] = biguid;
			}
			data.set(obj, map_name_guid);
			continue;
		}

		if (mid == entt::type_hash<std::vector<std::uint32_t>>::value()) {
			auto vecnode = in[field_name];
			std::vector<std::uint32_t> vec_uint{};
			for (auto const& value : vecnode) {
				vec_uint.emplace_back(value.template as<std::uint32_t>());
			}
			data.set(obj, vec_uint);
			continue;
		}

		if (mid == entt::type_hash<std::vector<std::string>>::value()) {
			auto vecnode = in[field_name];
			std::vector<std::string> vec_str{};
			for (auto const& value : vecnode) {
				vec_str.emplace_back(value.template as<std::string>());
			}
			data.set(obj, vec_str);
			continue;
		}

		if (mid == entt::type_hash<std::vector<rp::Guid>>::value()) {
			auto vecnode = in[field_name];
			std::vector<rp::Guid> vec_guid{};
			for (auto const& value : vecnode) {
				vec_guid.emplace_back(rp::Guid::to_guid(value.template as<std::string>()));
			}
			data.set(obj, vec_guid);
			continue;
		}

		// MaterialOverridesComponent map deserialization
		if (mid == entt::type_hash<std::unordered_map<std::string, float>>::value()) {
			auto mapnode = in[field_name];
			std::unordered_map<std::string, float> map_data{};
			for (auto const& pair : mapnode) {
				map_data[pair.first.template as<std::string>()] = pair.second.template as<float>();
			}
			data.set(obj, map_data);
			continue;
		}

		if (mid == entt::type_hash<std::unordered_map<std::string, glm::vec3>>::value()) {
			auto mapnode = in[field_name];
			std::unordered_map<std::string, glm::vec3> map_data{};
			auto vec3_meta_type = entt::resolve<glm::vec3>();
			for (auto const& pair : mapnode) {
				entt::meta_any vec3_any = vec3_meta_type.construct();
				DeserializeType(pair.second, vec3_any);
				glm::vec3 vec3_value = vec3_any.cast<glm::vec3>();
				map_data[pair.first.template as<std::string>()] = vec3_value;
			}
			data.set(obj, map_data);
			continue;
		}

		if (mid == entt::type_hash<std::unordered_map<std::string, glm::vec4>>::value()) {
			auto mapnode = in[field_name];
			std::unordered_map<std::string, glm::vec4> map_data{};
			auto vec4_meta_type = entt::resolve<glm::vec4>();
			for (auto const& pair : mapnode) {
				entt::meta_any vec4_any = vec4_meta_type.construct();
				DeserializeType(pair.second, vec4_any);
				glm::vec4 vec4_value = vec4_any.cast<glm::vec4>();
				map_data[pair.first.template as<std::string>()] = vec4_value;
			}
			data.set(obj, map_data);
			continue;
		}

		if (mid == entt::type_hash<std::unordered_map<std::string, glm::mat4>>::value()) {
			auto mapnode = in[field_name];
			std::unordered_map<std::string, glm::mat4> map_data{};
			auto mat4_meta_type = entt::resolve<glm::mat4>();
			for (auto const& pair : mapnode) {
				entt::meta_any mat4_any = mat4_meta_type.construct();
				DeserializeType(pair.second, mat4_any);
				glm::mat4 mat4_value = mat4_any.cast<glm::mat4>();
				map_data[pair.first.template as<std::string>()] = mat4_value;
			}
			data.set(obj, map_data);
			continue;
		}

		// Handle primitive types by type_hash ID
		if (mid == entt::type_hash<int>::value()) {
			data.set(obj, in[field_name].template as<int>());
		}
		else if (mid == entt::type_hash<float>::value()) {
			data.set(obj, in[field_name].template as<float>());
		}
		else if (mid == entt::type_hash<double>::value()) {
			data.set(obj, in[field_name].template as<double>());
		}
		else if (mid == entt::type_hash<std::string>::value()) {
			data.set(obj, in[field_name].template as<std::string>());
		}
		else if (mid == entt::type_hash<bool>::value()) {
			data.set(obj, in[field_name].template as<bool>());
		}
		else if (mid == entt::type_hash<uint32_t>::value()) {
			data.set(obj, in[field_name].template as<uint32_t>());
		}
	}
}


//templated deserialiser, Node must overload[](std::string const&)
template<typename Node>
void DeserializeEntity(entt::registry& reg, const Node& entity_node, entt::entity* enttptr = nullptr) {
	auto e = reg.create();
	if (enttptr)
		*enttptr = e;
	//const auto& components = entity_node["Entity"];
	const auto& components = entity_node;

	for (auto& [type_id, meta_type] : ReflectionRegistry::types()) {
		std::string comp_name = ReflectionRegistry::GetTypeName(meta_type.id());
		if (components[comp_name]) {
			entt::meta_any comp = meta_type.construct();
			DeserializeType(components[comp_name], comp);

			meta_type.func("emplace_meta_any"_tn).invoke({}, entt::forward_as_meta(reg), e, entt::forward_as_meta(comp));
		}
	}
}

struct InterfaceRegistrationBasic {};

template <typename Ret, typename ...Args>
struct FunctionTraits;

template <typename Ret, typename ...Args>
struct FunctionTraits<Ret(*)(Args...)> {
	using RetType = Ret;
	using ArgType = std::tuple<Args...>;
};

template<auto Ptr, static_string Name>
struct MemberRegistration {
	static constexpr auto ptr = Ptr;
	static constexpr std::string_view name = Name.data;
	static constexpr auto hash = ToTypeName(name);
};

template<auto SetPtr, auto GetPtr, static_string Name>
struct InterfaceRegistration : public InterfaceRegistrationBasic {
	static constexpr auto setptr = SetPtr;
	static constexpr auto getptr = GetPtr;
	static constexpr std::string_view name = Name.data;
	static constexpr auto hash = ToTypeName(name);
};

template<auto Ptr, static_string Name>
constexpr auto MemberRegistrationV = MemberRegistration<Ptr, Name>{};

template<auto SetPtr, auto GetPtr, static_string Name>
constexpr auto InterfaceRegistrationV = InterfaceRegistration<SetPtr, GetPtr, Name>{};

template<typename T>
void RegisterDataMember(auto& factory) {
	if constexpr (std::is_base_of_v<InterfaceRegistrationBasic, T>) {
		factory.template data<T::setptr, T::getptr>(T::hash);
	}
	else {
		factory.template data<T::ptr, entt::as_ref_t>(T::hash);
	}
}


template<typename T, typename... Refs>
void RegisterReflectionComponent(std::string_view type_name, Refs...) {
    auto hashtypename = ToTypeName(type_name);
    auto factory = entt::meta_factory<T>().type(hashtypename);
    
    auto& field_table{ ReflectionRegistry::RegisterType(hashtypename, type_name) };
    // populate field tables
    (field_table.try_emplace(Refs::hash, std::string(Refs::name)), ...);
   
    (RegisterDataMember<Refs>(factory), ...);

	factory.template func<&entt::registry::emplace<T>, entt::as_ref_t>("emplace"_tn);
	factory.template func< [](entt::registry& r, entt::entity e, entt::meta_any meta_any) {r.emplace<T>(e, meta_any.cast<T>()); } > ("emplace_meta_any"_tn);
	ReflectionRegistry::types()[entt::type_hash<T>::value()] = entt::resolve(hashtypename);
	ReflectionRegistry::InternalID()[entt::type_index<T>::value()] = entt::type_hash<T>::value();
	ReflectionRegistry::BinSerializerRegistryInstance().push_back({
		[](entt::snapshot& snap, std::ostream& os) {
			auto out_archive{ [&os](auto const& value) {
				os.write(reinterpret_cast<const char*>(&value), sizeof(value));
				} };
			snap.template get<T>(out_archive); },
		[](entt::snapshot_loader& loader, std::istream& is) {
			auto in_archive{ [&is](auto& value) {
				is.read(reinterpret_cast<char*>(&value), sizeof(value));
			} };
			loader.template get<T>(in_archive);
		} });
}

#define RegisterReflectionTypeBegin(TYPE, TYPENAME) \
namespace {											\
	inline int TYPE##_reflection_register = []() {	\
		RegisterReflectionComponent<TYPE>(TYPENAME,

#define RegisterReflectionTypeEnd	\
		); return 1;				\
	}();							\
}
#endif
