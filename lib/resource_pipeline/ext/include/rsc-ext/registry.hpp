#ifndef RP_RSC_EXT_REGISTRY_HPP
#define RP_RSC_EXT_REGISTRY_HPP

#include <yaml-cpp/yaml.h>
#include <rsc-core/registry.hpp>
#include "rsc-ext/importer.hpp"
#include "rsc-ext/descriptor.hpp"
#include "rsc-core/guid.hpp"

namespace rp {
	struct TypeUnsafeTypeErasedWrapper {
		std::unique_ptr<std::byte[]> m_data;
		TypeUnsafeTypeErasedWrapper() = delete;
		template <typename Type>
		TypeUnsafeTypeErasedWrapper(Type&& v) : m_data{ std::make_unique<std::byte[]>(sizeof(Type)) } {
			std::memcpy(m_data.get(), &v, sizeof(Type));
		}
		TypeUnsafeTypeErasedWrapper(TypeUnsafeTypeErasedWrapper&& w) : m_data{ std::move(w.m_data) } {}
		~TypeUnsafeTypeErasedWrapper() = default;
		operator std::byte* () {
			return m_data.get();
		}
	};

	struct DescriptorWrapper {
		TypeUnsafeTypeErasedWrapper m_wrap;
		std::uint64_t m_desc_importer_hash;
		DescriptorWrapper(DescriptorWrapper const&) = delete;
		DescriptorWrapper(DescriptorWrapper&&) = default;
		operator std::byte* () {
			return m_wrap.m_data.get();
		}
	};

	//handles management and registration of resource type importers
	struct ResourceTypeImporterRegistry {
	private:
		ResourceTypeImporterRegistry() = default;

		std::unordered_map<std::string, std::uint64_t> m_known_file_ext_importer;
		std::unordered_map<std::uint64_t, std::function<void(std::string const&)>> m_importers;
		std::unordered_map<std::uint64_t, std::function<TypeUnsafeTypeErasedWrapper()>> m_descriptor_factory;
		std::unordered_multimap<std::uint64_t,std::pair<std::string, std::function<void(std::string const&, std::byte*)>>> m_serializers;

		static std::unique_ptr<ResourceTypeImporterRegistry>& InstancePtr() {
			static std::unique_ptr<ResourceTypeImporterRegistry> s_inst_ptr{ new ResourceTypeImporterRegistry() };
			return s_inst_ptr;
		}

		static ResourceTypeImporterRegistry& Instance() {
			return *InstancePtr();
		}
	public:
		static void Serialize(std::uint64_t typehash, std::string const& serializer_name, std::string const& file_name, std::byte* data) {
			auto serializer_range{Instance().m_serializers.equal_range(typehash)};
			for (auto it = serializer_range.first; it != serializer_range.second; ++it) {
				if (it->second.first == serializer_name) {
					it->second.second(file_name, data);
					return;
				}
			}
		}
		static void RegisterImporter(std::uint64_t typehash, std::function<void(std::string const&)> fn) {
			Instance().m_importers.emplace(typehash, fn);
		}
		static void RegisterSerializer(std::uint64_t typehash, std::string const& serializer_name, std::function<void(std::string const&, std::byte*)> fn) {
			Instance().m_serializers.emplace(typehash, std::pair<std::string, std::function<void(std::string const&, std::byte*)>>{serializer_name, fn});
		}
		static void RegisterFactory(std::uint64_t typehash, std::function<TypeUnsafeTypeErasedWrapper()> fn) {
			Instance().m_descriptor_factory.emplace(typehash, fn);
		}
		static void Import(std::uint64_t typehash, std::string const& file_name) {
			auto& imp{ Instance().m_importers };
			auto res{ imp.find(typehash) };
			assert(res != imp.end() && "importer not registered or not found");
			res->second(file_name);
		}
		static TypeUnsafeTypeErasedWrapper CreateDescriptor(std::uint64_t typehash) {
			auto& fac{ Instance().m_descriptor_factory };
			auto res{ fac.find(typehash) };
			assert(res != fac.end() && "type not registered or not found");
			return res->second();
		}
	};
}

namespace rp {
	namespace serialization {
		template <>
		struct in_archive<"yaml"> {
			YAML::Node nd;
			in_archive() = delete;
			in_archive(std::string const& filepath) : nd{ YAML::LoadFile(filepath) } {}
			~in_archive() {}
			template <typename Type>
			Type read(YAML::Node const& cnd) {
				if constexpr (std::is_enum_v<Type>) {
					return reflection::map_enum_value<Type>(cnd.as<std::string>());
				}
				else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, rp::Guid>) {
					return rp::Guid::to_guid(cnd.as<std::string>());
				}
				else if constexpr (std::is_fundamental_v<Type> || std::is_same_v<std::remove_cvref_t<Type>, std::string>) {
					return cnd.as<Type>();
				}
				else if constexpr (reflection::is_associative_container_v<Type>) {
					Type v{};
					if (cnd.IsMap()) {
						for (auto it = cnd.begin(); it != cnd.end(); ++it) {
							v[read<typename Type::key_type>(it->first)] = read<typename Type::mapped_type>(it->second);
						}
					}
					return v;
				}
				else if constexpr (reflection::is_sequence_container_v<Type>) {
					return cnd.as<Type>();
				}
				else if constexpr (std::is_class_v<Type>) {
					Type v{};
					reflection::reflect(v).each([&](auto& field) {
						*field.m_field_ptr = read<std::remove_pointer_t<std::remove_cvref_t<decltype(field.m_field_ptr)>>>(cnd[std::string(field.m_field_name.begin(), field.m_field_name.end())]);
						});
					return v;
				}
				else {
					return Type{};
				}
			}
			template <typename Type>
			Type read() {
				return read<Type>(nd);
			}
		};

		template <>
		struct out_archive<"yaml"> {
			std::ofstream m_ofs;
			YAML::Node out;
			out_archive() = delete;
			out_archive(std::string const& filepath) : m_ofs{ filepath } {}
			~out_archive() { m_ofs << out; m_ofs.close(); }
			template <typename Type>
			void write(Type const& v, YAML::Node& nd) {
				if constexpr (std::is_enum_v<Type>) {
					nd = std::string(reflection::map_enum_name(v));
				}
				else if constexpr (std::is_same_v<std::remove_cvref_t<Type>, rp::Guid>) {
					nd = v.to_hex();
				}
				else if constexpr (std::is_fundamental_v<Type> || std::is_same_v<std::remove_cvref_t<Type>, std::string>) {
					nd = v;
				}
				else if constexpr (reflection::is_associative_container_v<Type> || reflection::is_sequence_container_v<Type>) {
					auto cont{ reflection::reflect(v) };
					cont.each([&](auto const& field) {
						YAML::Node member{};
						write(field.second, member);
						nd[field.first] = member;
						});
				}
				else if constexpr (std::is_class_v<Type>) {
					reflection::reflect(v).each([&](auto const& field) {
						YAML::Node member{};
						write(*field.m_field_ptr, member);
						nd[std::string(field.m_field_name.begin(), field.m_field_name.end())] = member;
						});
				}
			}
			template <typename Type>
			void write(Type const& v) {
				write(v, out);
			}
		};

		template <>
		struct serializer<"yaml"> {
			using in_archive_type = in_archive<"yaml">;
			using out_archive_type = out_archive<"yaml">;
			template <typename Type>
			static void serialize(Type const& val, std::string const& arcpath) {
				out_archive_type{ arcpath }.write(val);
			}
			template <typename Type>
			static void serialize(Type const& val, out_archive_type& arc) {
				arc.write(val);
			}
			template <typename Type>
			static void serialize(Type const& val, out_archive_type&& arc) {
				arc.write(val);
			}
			template <typename Type>
			[[nodiscard]] static Type deserialize(std::string const& arcpath) {
				return in_archive_type{ arcpath }.read<Type>();
			}
			template <typename Type>
			[[nodiscard]] static Type deserialize(in_archive_type& arc) {
				return arc.read<Type>();
			}
			template <typename Type>
			[[nodiscard]] static Type deserialize(in_archive_type&& arc) {
				return arc.read<Type>();
			}
		};

		using yaml_serializer = serializer<"yaml">;
	}
}

//put header guards if this is declared in header
//RegisterResourceTypeImporter(descriptor, native, importer, known aliases...)
#define RegisterResourceTypeImporter(DESC, NATIVETYPE, NATIVETYPESUFFIX, IMPORTER, ...)		\
template <> struct rp::ResourceTypeImporter<rp::utility::type_hash<DESC>::value()>{			\
	using type = NATIVETYPE;																\
	static constexpr auto type_hash{ rp::utility::type_hash<DESC>::value() };				\
	inline static type Import(std::string const& path) {									\
		return IMPORTER(rp::serialization::yaml_serializer::deserialize<DESC>(path));		\
	}																						\
	inline static DESC GetDescriptor(std::string const& path) {								\
		return rp::serialization::yaml_serializer::deserialize<DESC>(path);					\
	}																						\
	inline static void ImportSerialize(std::string const& path) {							\
		rp::serialization::binary_serializer::serialize(Import(path), GetDescriptor(path).base.m_guid.to_hex() + NATIVETYPESUFFIX);		\
	}																						\
};																							\
namespace {																						\
	inline const std::uint32_t registrar##DESC{ [] {											\
		static constexpr auto type_hash{ rp::utility::type_hash<DESC>::value() };				\
		rp::ResourceTypeImporterRegistry::RegisterImporter(type_hash, rp::ResourceTypeImporter<rp::utility::type_hash<DESC>::value()>::ImportSerialize);			\
		rp::ResourceTypeImporterRegistry::RegisterSerializer(type_hash, "yaml", [](std::string const& str, std::byte* data) {										\
			DESC& desc{ *reinterpret_cast<DESC*>(data) };																											\
			rp::serialization::yaml_serializer::serialize(desc, str);																								\
			});																																						\
		rp::ResourceTypeImporterRegistry::RegisterFactory(type_hash, []() -> rp::TypeUnsafeTypeErasedWrapper {return rp::TypeUnsafeTypeErasedWrapper{ DESC() }; });	\
		return 1u;																																					\
		}() };																																						\
}

#endif