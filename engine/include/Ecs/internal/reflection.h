#ifndef LIB_REFLECTION_REGISTRATION_H
#define LIB_REFLECTION_REGISTRATION_H

#include <entt/entt.hpp>
#include <vector>
#include <functional>
#include <iostream>
#include <fstream>

using TypeName = entt::hashed_string;
constexpr TypeName ToTypeName(std::string_view name) { return entt::hashed_string(name.data(), name.length()); };

constexpr TypeName operator""_tn(const char* str, std::size_t len) noexcept {
    return entt::hashed_string(str, len);
}

decltype(auto) ResolveType(TypeName t_name) {
    return entt::resolve(t_name);
}

struct ComponentBinarySerializer {
    std::function<void(entt::snapshot&, std::ostream&)> m_Saver;
    std::function<void(entt::snapshot_loader&, std::istream&)> m_Loader;
};

struct ReflectionRegistry {
    using TypeID = entt::id_type; // same as std::uint32_t

    struct Detail {
        std::unordered_map<TypeID, entt::meta_type> m_Storage;
        std::vector<ComponentBinarySerializer> m_BinRegistry;
    };

    static ReflectionRegistry::Detail& Registry() {
        static ReflectionRegistry::Detail instance;
        return instance;
    }

    static std::unordered_map<TypeID, entt::meta_type>& types() {
        return Registry().m_Storage;
    }
    static std::vector<ComponentBinarySerializer> BinSerializerRegistryInstance() {
        return Registry().m_BinRegistry;
    }
};

//templated serialiser, Node must overload[]
//serialise type recursively using reflection (impt: user defined type and user defined nested member types must be registered to work properly)
template <typename Node>
void SerializeType(const entt::meta_any &obj, Node&out) {
    auto type = obj.type();

    for (auto data : type.data()) {
        auto name_prop = data.prop("name"_tn);
        std::string field_name = name_prop ? name_prop.value().cast<std::string>()
                                           : std::string(data.id().data());

        auto value = data.get(obj);
        
        if (value.type().data().begin() != value.type().data().end()) {
            Node nested;
            SerializeType(value, nested);
            out[field_name] = nested;
        } 
        else {
            // primitives
            if (value.template can_cast<int>())
                out[field_name] = value.cast<int>();
            else if (value.template can_cast<float>())
                out[field_name] = value.cast<float>();
            else if (value.template can_cast<double>())
                out[field_name] = value.cast<double>();
            else if (value.template can_cast<std::string>())
                out[field_name] = value.cast<std::string>();
        }
    }
}

//templated serialiser, Node must overload[](std::string const&)
template <typename Node>
Node serialize_entity(entt::registry& reg, entt::entity e) {
    Node entity_node;

    for (auto& [type_id, meta_type] : ReflectionRegistry::types()) {
        auto storage = reg.storage(type_id);
        if (storage && storage->contains(e)) {
            auto comp_any = storage->get(e);
            Node comp_node;
            SerializeType(comp_any, comp_node);
            entity_node[meta_type.info().name().data()] = comp_node;
        }
    }

    return entity_node;
}


/***********************/
/*        usage        */
/***********************/
// Syntax: ReflectComponent(ComponentType, .data<&ComponentType::Member>("AssociatedName"_tn), ...)
// eg: ReflectComponent(Position, .data<&Position::x>("x"_tn), .data<&Position::y>("y"_tn), .data<&Position::z>("z"_tn))
// just live with this macro
#define ReflectComponent(COMPONENT_TYPE, ...)                                                                            \
namespace {                                                                                                              \
    struct COMPONENT_TYPE##{                                                                                             \
        COMPONENT_TYPE##() {                                                                                             \
            entt::meta<COMPONENT_TYPE>().type(#COMPONENT_TYPE##_tn) __VA_ARGS__;                                         \
            ReflectionRegistry::BinSerializerRegistryInstance().push_back(                                               \
                [](entt::snapshot& snap, std::ostream& os) { snap.component<COMPONENT_TYPE>(os); },                      \
                [](entt::snapshot_loader& loader, std::istream& is) { loader.component<COMPONENT_TYPE>(is); }            \
            });                                                                                                          \
        }                                                                                                                \
    };                                                                                                                   \
}                                                                           

//macro free alternative. to use in code blocks no global space
template<typename T, typename... MemberPairs>
void RegisterReflectionComponent(std::string_view name, MemberPairs&&... members) {
    auto meta_type = entt::meta<T>().type(entt::hashed_string{ name.data() });
    (meta_type.data(members.first, members.second), ...);
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
}


#endif