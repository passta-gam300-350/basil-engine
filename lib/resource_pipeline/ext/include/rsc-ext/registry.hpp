#ifndef RP_RSC_EXT_REGISTRY_HPP
#define RP_RSC_EXT_REGISTRY_HPP

#include <yaml-cpp/yaml.h>
#include <rsc-core/rp.hpp>
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

		using NativeTypeId = std::uint64_t;
		using ImporterTypeId = std::uint64_t;

		std::unordered_map<std::string, ImporterTypeId> m_known_file_ext_importer;
		std::unordered_map<ImporterTypeId, NativeTypeId> m_importer_native_type;
		std::unordered_map<ImporterTypeId, std::string> m_importer_native_suffix;
		std::unordered_map<ImporterTypeId, std::function<void(std::string const&, std::string const&)>> m_importers;
		std::unordered_map<NativeTypeId, std::string> m_native_suffix;
		std::unordered_map<ImporterTypeId, std::function<TypeUnsafeTypeErasedWrapper()>> m_descriptor_factory;
		std::unordered_map<ImporterTypeId, std::function<DescriptorWrapper(std::string const&)>> m_descriptor_loader;
		std::unordered_multimap<ImporterTypeId,std::pair<std::string, std::function<void(std::string const&, std::byte*)>>> m_serializers;

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
		static void RegisterImporter(std::uint64_t typehash, std::function<void(std::string const&, std::string const&)> fn) {
			Instance().m_importers.emplace(typehash, fn);
		}
		static void RegisterSerializer(std::uint64_t typehash, std::string const& serializer_name, std::function<void(std::string const&, std::byte*)> fn) {
			Instance().m_serializers.emplace(typehash, std::pair<std::string, std::function<void(std::string const&, std::byte*)>>{serializer_name, fn});
		}
		static void RegisterFactory(std::uint64_t typehash, std::function<TypeUnsafeTypeErasedWrapper()> fn) {
			Instance().m_descriptor_factory.emplace(typehash, fn);
		}
		static void RegisterExtNameImporter(std::uint64_t typehash, std::string const& ext_name) {
			Instance().m_known_file_ext_importer.emplace(ext_name, typehash);
		}
		static void RegisterImporterNativeType(std::uint64_t typehash, std::uint64_t nativetypehash) {
			Instance().m_importer_native_type.emplace(typehash, nativetypehash);
		}
		static void RegisterImporterNativeSuffix(std::uint64_t typehash, std::string const& str) {
			Instance().m_importer_native_suffix.emplace(typehash, str);
		}
		static void Import(std::uint64_t typehash, std::string const& file_name, std::string const& out_name = {}) {
			auto& imp{ Instance().m_importers };
			auto res{ imp.find(typehash) };
			assert(res != imp.end() && "importer not registered or not found");
			res->second(file_name, out_name);
		}
		/*
		static void Import(DescriptorWrapper& desc) {
			auto& imp{ Instance().m_importers };
			auto res{ imp.find(desc.m_desc_importer_hash) };
			assert(res != imp.end() && "importer not registered or not found");
			res->second(file_name);
		}*/
		static std::uint64_t GetDescriptorImporterType(std::string const& str) {
			YAML::Node const nd{ YAML::LoadFile(str) };
			return nd["base"]["m_importer_type"].as<std::uint64_t>();
		}
		static std::string GetDescriptorName(std::string const& str) {
			YAML::Node const nd{ YAML::LoadFile(str) };
			return nd["base"]["m_name"].as<std::string>();
		}
		static std::string GetResourceExt(std::uint64_t native_it) {
			auto it{ Instance().m_native_suffix.find(native_it) };
			return it != Instance().m_native_suffix.end() ? it->second : "";
		}
		static rp::BasicIndexedGuid GetDescriptorGuid(std::string const& str) {
			YAML::Node const nd{ YAML::LoadFile(str) };
			auto it{ Instance().m_importer_native_type.find(GetDescriptorImporterType(str)) };
			return it == Instance().m_importer_native_type.end() ? rp::null_indexed_guid : rp::BasicIndexedGuid{rp::Guid::to_guid(nd["base"]["m_guid"].as<std::string>()), it->second};
		}
		static void SetDescriptorBase(rp::descriptor_base desc_base, std::string const& str) {
			YAML::Node nd{ YAML::LoadFile(str) };
			nd["base"]["m_guid"] = desc_base.m_guid.to_hex();
			nd["base"]["m_importer"] = desc_base.m_importer;
			nd["base"]["m_name"] = desc_base.m_name;
			nd["base"]["m_importer_type"] = desc_base.m_importer_type;
			nd["base"]["m_source"] = desc_base.m_source;
			std::ofstream ofs{str};
			ofs << nd;
		}
		static void SetDescriptorBaseSource(std::string const& src, std::string const& str) {
			YAML::Node nd{ YAML::LoadFile(str) };
			nd["base"]["m_source"] = src;
			std::ofstream ofs{ str };
			ofs << nd;
		}
		static rp::descriptor_base GetDescriptorBase(std::string const& str) {
			YAML::Node const nd{ YAML::LoadFile(str) };
			rp::descriptor_base desc_base;
			desc_base.m_guid = rp::Guid::to_guid(nd["base"]["m_guid"].as<std::string>());
			desc_base.m_importer = nd["base"]["m_importer"].as<std::string>();
			desc_base.m_name = nd["base"]["m_name"].as<std::string>();
			desc_base.m_importer_type = nd["base"]["m_importer_type"].as<std::uint64_t>();
			desc_base.m_source = nd["base"]["m_source"].as<std::string>();
			return desc_base;
		}
		static std::string GetImporterSuffix(std::uint64_t typehash) {
			return Instance().m_importer_native_suffix[typehash];
		}
		static TypeUnsafeTypeErasedWrapper CreateDescriptor(std::uint64_t typehash) {
			auto& fac{ Instance().m_descriptor_factory };
			auto res{ fac.find(typehash) };
			assert(res != fac.end() && "type not registered or not found");
			return res->second();
		}
		static DescriptorWrapper LoadDescriptor(std::string const& str) {
			auto& ldr{ Instance().m_descriptor_loader };
			std::size_t imp_id{ GetDescriptorImporterType(str) };
			auto res{ ldr.find(imp_id) };
			assert(res != ldr.end() && "type not registered or not found");
			return res->second(str);
		}
		static void CreateDefaultDescriptor(std::string const& str) {
			auto pos{ str.rfind('.') };
			std::string ext{ pos == std::string::npos ? "" : str.substr(pos) };
			std::string desc_name{ str.substr(0, pos) + ".desc" };
			auto imp{ Instance().m_known_file_ext_importer };
			std::uint64_t descimpid{};
			for (auto& [xname, imp_id] : imp) {
				if (xname == ext) {
					descimpid = imp_id;
					break;
				}
			}
			if (descimpid) {
				auto d{ CreateDescriptor(descimpid) };
				Serialize(descimpid, "yaml", desc_name, d);
				rp::descriptor_base desc{ GetDescriptorBase(desc_name) };
				desc.m_source = str;
				desc.m_name = str.substr(str.rfind('\\') + 1);
				SetDescriptorBase(desc, desc_name);
			}
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

#include <iostream>

//put header guards if this is declared in header
//RegisterResourceTypeImporter(descriptor, native, importer, known aliases...) match NATIVETYPENAME with RegisterResourceType
#define RegisterResourceTypeImporter(DESC, NATIVETYPE, NATIVETYPENAME, NATIVETYPESUFFIX, IMPORTER, ...)		\
template <> struct rp::ResourceTypeImporter<rp::utility::type_hash<DESC>::value()>{			\
	using type = NATIVETYPE;																\
	static constexpr auto type_hash{ rp::utility::type_hash<DESC>::value() };				\
	inline static auto Import(std::string const& path) {									\
		return IMPORTER(rp::serialization::yaml_serializer::deserialize<DESC>(path));		\
	}																						\
	inline static DESC GetDescriptor(std::string const& path) {								\
		return rp::serialization::yaml_serializer::deserialize<DESC>(path);					\
	}																						\
	inline static void ImportSerialize(std::string const& path, std::string const& out) {	\
		auto data{ Import(path) };															\
		[](auto const& ds, std::string const& str){															\
			if constexpr (rp::reflection::is_sequence_container_v<std::remove_cvref_t<decltype(ds)>>) {		\
				auto parent{str.substr(0,str.rfind('\\')+1)};												\
				for (auto& [guid, d] : ds) {																\
					rp::serialization::binary_serializer::serialize(d, parent + guid.to_hex() + NATIVETYPESUFFIX);	\
					assert(std::filesystem::exists(parent + guid.to_hex() + NATIVETYPESUFFIX));				\
				}																							\
			}																								\
			else {																							\
				rp::serialization::binary_serializer::serialize(ds, str);									\
			}																								\
		}(data, out.empty() ? GetDescriptor(path).base.m_guid.to_hex() + NATIVETYPESUFFIX : out);			\
	}																						\
	inline static void ImportSerializeDirect(DESC const& desc) {							\
		auto data{ IMPORTER(desc) };														\
		[](auto const& ds, std::string const& str){															\
			if constexpr (rp::reflection::is_sequence_container_v<std::remove_cvref_t<decltype(ds)>>) {		\
				for (auto& [guid, d] : ds) {																\
					rp::serialization::binary_serializer::serialize(d, guid.to_hex() + NATIVETYPESUFFIX);	\
				}																							\
			}																								\
			else {																							\
				rp::serialization::binary_serializer::serialize(ds, str);									\
			}																								\
		}(data, desc.base.m_guid.to_hex() + NATIVETYPESUFFIX);												\
	}																										\
};																								\
namespace {																						\
	inline const std::uint32_t registrar##DESC{ [] {											\
		static constexpr auto type_hash{ rp::utility::type_hash<DESC>::value() };				\
		rp::ResourceTypeImporterRegistry::RegisterImporter(type_hash, rp::ResourceTypeImporter<rp::utility::type_hash<DESC>::value()>::ImportSerialize);			\
		rp::ResourceTypeImporterRegistry::RegisterSerializer(type_hash, "yaml", [](std::string const& str, std::byte* data) {										\
			DESC& desc{ *reinterpret_cast<DESC*>(data) };																											\
			rp::serialization::yaml_serializer::serialize(desc, str);																								\
			});																																						\
		rp::ResourceTypeImporterRegistry::RegisterFactory(type_hash, []() -> rp::TypeUnsafeTypeErasedWrapper {DESC desc{}; desc.base.m_guid = rp::Guid::generate(); desc.base.m_importer = NATIVETYPESUFFIX; desc.base.m_importer_type = rp::utility::type_hash<DESC>::value(); return rp::TypeUnsafeTypeErasedWrapper{ desc }; });	\
		auto ext_names{std::array{__VA_ARGS__}};																													\
		for (auto& ext : ext_names) {																																\
			rp::ResourceTypeImporterRegistry::RegisterExtNameImporter(type_hash, ext);																				\
		}																																							\
		rp::ResourceTypeImporterRegistry::RegisterImporterNativeType(type_hash, rp::utility::string_hash(NATIVETYPENAME));											\
		rp::ResourceTypeImporterRegistry::RegisterImporterNativeSuffix(type_hash, NATIVETYPESUFFIX);																\
		return 1u;																																					\
		}() };																																						\
}

#endif