#ifndef LIB_REFLECTION_REGISTRATION_H
#define LIB_REFLECTION_REGISTRATION_H
#include "serialisation/guid.h"
#include <entt/entt.hpp>
#include <entt/meta/meta.hpp>
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
		any.type().data().begin();
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
			
			if (Resource::Guid* v = value.try_cast<Resource::Guid>())
				out[field_name] = v->to_hex_no_delimiter();
			

			// primitives
			else if (int* v = value.try_cast<int>())
				out[field_name] = *v;
			else if (float* v = value.try_cast<float>())
				out[field_name] = *v;
			else if (double* v = value.try_cast<double>())
				out[field_name] = *v;
			else if (std::string* v = value.try_cast<std::string>())
				out[field_name] = *v;
			else if (bool* v = value.try_cast<bool>())
				out[field_name] = *v;
		

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

		if (mid == entt::type_hash<Resource::Guid>::value()) {
			std::string guid_str = in[field_name].template as<std::string>();
			Resource::Guid guid = Resource::Guid::to_guid(guid_str);
			data.set(obj, guid);
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
	}
}



//templated deserialiser, Node must overload[](std::string const&)
template<typename Node>
void DeserializeEntity(entt::registry& reg, const Node& entity_node) {
	auto e = reg.create();
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

	//static_assert(FunctionTraits<decltype(SetPtr)>::);
	//static_assert();
};

template<auto Ptr, static_string Name>
constexpr auto MemberRegistrationV = MemberRegistration<Ptr, Name>{};

template<auto SetPtr, auto GetPtr, static_string Name>
constexpr auto InterfaceRegistrationV = InterfaceRegistration<SetPtr, GetPtr, Name>{};

template<typename T>
void RegisterDataMember(auto& factory) {
	if constexpr (std::is_base_of_v<InterfaceRegistrationBasic, T>) {
		factory.data<T::setptr, T::getptr>(T::hash);
	}
	else {
		factory.data<T::ptr>(T::hash);
	}
}


template<typename T, typename... Refs>
void RegisterReflectionComponent(std::string_view type_name, Refs...) {
	auto hashtypename = ToTypeName(type_name);
	auto factory = entt::meta<T>().type(hashtypename);

	auto& field_table{ ReflectionRegistry::RegisterType(hashtypename, type_name) };
	// populate field tables
	(field_table.try_emplace(Refs::hash, std::string(Refs::name)), ...);

	// For each Refs (each is a MemberRegistration<…, …>):
	(RegisterDataMember<Refs>(factory), ...);

	factory.func<&entt::registry::emplace<T>, entt::as_ref_t>("emplace"_tn);
	factory.func < [](entt::registry& r, entt::entity e, entt::meta_any meta_any) {r.emplace<T>(e, meta_any.cast<T>()); } > ("emplace_meta_any"_tn);
	ReflectionRegistry::types()[entt::type_hash<T>::value()] = entt::resolve(hashtypename);
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

/***********************/
/*        usage        */
/***********************/
// Syntax: ReflectComponent(ComponentType, .data<&ComponentType::Member>("AssociatedName"_tn), ...)
// eg: ReflectComponent(Position, .data<&Position::x>("x"_tn), .data<&Position::y>("y"_tn), .data<&Position::z>("z"_tn))
// just live with this macro
#define ReflectComponent(COMPONENT_TYPE, ...)                                                                            \
namespace {                                                                                                              \
    struct COMPONENT_TYPE##_Registration{                                                                                 \
        COMPONENT_TYPE##_Registration() {                                                                                \
            entt::meta<COMPONENT_TYPE>().type(#COMPONENT_TYPE##_tn) __VA_ARGS__;                                         \
            entt::meta<COMPONENT_TYPE>().func<&entt::registry::emplace<COMPONENT_TYPE>, entt::as_void_t>("emplace"_tn);  \
            ReflectionRegistry::BinSerializerRegistryInstance().push_back({                                              \
                [](entt::snapshot& snap, std::ostream& os) {                                                             \
                    auto out_archive{ [&os](auto value) {                                                                \
                        os.write(reinterpret_cast<const char*>(&value), sizeof(value));                                  \
                    } };                                                                                                 \
                    snap.get<COMPONENT_TYPE>(out_archive); },                                                            \
                [](entt::snapshot_loader& loader, std::istream& is) {                                                    \
                    auto in_archive{ [&is](auto value) {                                                                 \
                    is.read(reinterpret_cast<char*>(&value), sizeof(value));                                             \
                    } };                                                                                                 \
                    loader.get<COMPONENT_TYPE>(in_archive); }                                                            \
            });                                                                                                          \
        }                                                                                                                \
    };                                                                                                                   \
    static COMPONENT_TYPE##_Registration COMPONENT_TYPE##RegisterV{}                                                     \
}                                                                           

#define ReflectComponentLocal(COMPONENT_TYPE, ...)                                                                   \
struct COMPONENT_TYPE##_Registration{                                                                                \
    COMPONENT_TYPE##_Registration() {                                                                                \
        entt::meta<COMPONENT_TYPE>().type(#COMPONENT_TYPE##_tn) __VA_ARGS__;                                         \
        entt::meta<COMPONENT_TYPE>().func<&entt::registry::emplace<COMPONENT_TYPE>, entt::as_void_t>("emplace"_tn);  \
        ReflectionRegistry::BinSerializerRegistryInstance().push_back({                                              \
            [](entt::snapshot& snap, std::ostream& os) {                                                             \
                auto out_archive{ [&os](auto value) {                                                                \
                    os.write(reinterpret_cast<const char*>(&value), sizeof(value));                                  \
                } };                                                                                                 \
                snap.get<COMPONENT_TYPE>(out_archive); },                                                            \
            [](entt::snapshot_loader& loader, std::istream& is) {                                                    \
                auto in_archive{ [&is](auto value) {                                                                 \
                is.read(reinterpret_cast<char*>(&value), sizeof(value));                                             \
                } };                                                                                                 \
                loader.get<COMPONENT_TYPE>(in_archive); }                                                            \
        });                                                                                                          \
    }                                                                                                                \
    static COMPONENT_TYPE##_Registration COMPONENT_TYPE##RegisterV{}                                                     \
};                                                                                                                                                                                              


#define ReflectEnum(ENUM_TYPE, ...)                                                                \
namespace {                                                                                        \
    struct EnumRegistration {                                                                      \
        EnumRegistration() {                                                                       \
            entt::meta<ENUM_TYPE>()                                                                \
                .type(#ENUM_TYPE##_tn)                                                             \
                .enum_<ENUM_TYPE>()                                                                \
                __VA_ARGS__                                                                        \
                .conv<std::underlying_type_t<ENUM_TYPE>>();                                        \
            ReflectionRegistry::types()[entt::type_hash<ENUM_TYPE>::value()] =                     \
                entt::resolve(#ENUM_TYPE##_tn);                                                    \
            ReflectionRegistry::TypeNames()[entt::type_hash<ENUM_TYPE>::value()] = #ENUM_TYPE;     \
        }                                                                                          \
    };                                                                                             \
    static EnumRegistration EnumRegistrationInstance{};                                            \
}


/*
//macro free alternative. to use in code blocks no global space
template<typename T, typename... MemberPairs>
void RegisterReflectionComponent(std::string_view name, MemberPairs&&... members) {
	auto meta_type = entt::meta<T>().type(entt::hashed_string{ name.data() });
	(meta_type.data<typename AssociatedMemberTraits<std::decay_t<MemberPairs>>::MemberPointer>(members.second), ...);
	meta_type.func<&entt::registry::emplace<T>, entt::as_void_t>("emplace"_tn);
	ReflectionRegistry::types()[entt::type_hash<T>::value()] = meta_type;
	ReflectionRegistry::BinSerializerRegistryInstance().push_back({
		[](entt::snapshot& snap, std::ostream& os) {
			auto out_archive{ [&os](auto value) {
				os.write(reinterpret_cast<const char*>(&value), sizeof(value));
				} };
			snap.template get<T>(out_archive); },
		[](entt::snapshot_loader& loader, std::istream& is) {
			auto in_archive{ [&is](auto value) {
			is.read(reinterpret_cast<char*>(&value), sizeof(value));
			} };
			loader.template get<T>(in_archive);
		}});
}*/


#endif