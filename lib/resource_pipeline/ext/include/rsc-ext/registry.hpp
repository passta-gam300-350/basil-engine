#ifndef RP_RSC_EXT_REGISTRY_HPP
#define RP_RSC_EXT_REGISTRY_HPP

#include <yaml-cpp/yaml.h>
#include <rsc-core/rp.hpp>
#include "rsc-ext/importer.hpp"
#include "rsc-ext/descriptor.hpp"
#include "rsc-core/guid.hpp"

namespace rp {
	inline namespace v2 {
		struct TypeUnsafeTypeErasedWrapper {
			std::byte* m_data;
			std::function<void(void)> m_deleter;
			TypeUnsafeTypeErasedWrapper() = delete;
			template <typename Type>
			TypeUnsafeTypeErasedWrapper(Type* v, std::function<void(void)> del) : m_data{ reinterpret_cast<std::byte*>(v) }, m_deleter{ del } {}
			TypeUnsafeTypeErasedWrapper(TypeUnsafeTypeErasedWrapper&& w) : m_data{ w.m_data }, m_deleter{ w.m_deleter } {
				w.m_deleter = nullptr;
			}
			~TypeUnsafeTypeErasedWrapper() {
				if (m_deleter) {
					m_deleter();
				}
			};
			operator std::byte* () {
				return m_data;
			}
		};
	}
	/*namespace v1 {
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
	}*/

	struct DescriptorWrapper {
		TypeUnsafeTypeErasedWrapper m_wrap;
		std::uint64_t m_desc_importer_hash;
		DescriptorWrapper(DescriptorWrapper const&) = delete;
		DescriptorWrapper(DescriptorWrapper&&) = default;
		template <typename Type>
		DescriptorWrapper(Type&& v, std::uint64_t importer_hash) : m_wrap{ std::forward<Type>(v) }, m_desc_importer_hash { importer_hash } {}
		operator std::byte* () {
			return m_wrap.m_data;
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
		static void RegisterLoader(std::uint64_t typehash, std::function<DescriptorWrapper(std::string const&)> fn) {
			Instance().m_descriptor_loader.emplace(typehash, fn);
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
		static void RegisterNativeSuffix(std::uint64_t native_type_hash, std::string const& suffix) {
			Instance().m_native_suffix.emplace(native_type_hash, suffix);
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
		static void CreateDefaultDescriptor(std::string const& str, std::string const& relativepath = {}) {
			auto pos{ str.rfind('.') };
			std::string ext{ pos == std::string::npos ? "" : str.substr(pos) };
			std::string desc_name{ str + ".desc" };
			auto imp{ Instance().m_known_file_ext_importer };
			[[maybe_unused]] auto& inst{ Instance() };
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
				desc.m_source = rp::utility::get_relative_path(str, relativepath);
				desc.m_name = str.substr(str.rfind('\\') + 1);
				SetDescriptorBase(desc, desc_name);
			}
		}
	};
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
		rp::ResourceTypeImporterRegistry::RegisterFactory(type_hash, []() -> rp::TypeUnsafeTypeErasedWrapper {DESC* desc{new DESC{}}; desc->base.m_guid = rp::Guid::generate(); desc->base.m_importer = NATIVETYPESUFFIX; desc->base.m_importer_type = rp::utility::type_hash<DESC>::value(); return rp::TypeUnsafeTypeErasedWrapper{ desc, [desc]{delete desc;} }; });	\
		auto ext_names{std::array{__VA_ARGS__}};																													\
		for (auto& ext : ext_names) {																																\
			rp::ResourceTypeImporterRegistry::RegisterExtNameImporter(type_hash, ext);																				\
		}																																							\
		rp::ResourceTypeImporterRegistry::RegisterImporterNativeType(type_hash, rp::utility::string_hash(NATIVETYPENAME));											\
		rp::ResourceTypeImporterRegistry::RegisterImporterNativeSuffix(type_hash, NATIVETYPESUFFIX);																\
		rp::ResourceTypeImporterRegistry::RegisterNativeSuffix(rp::utility::string_hash(NATIVETYPENAME), NATIVETYPESUFFIX);										\
		rp::ResourceTypeImporterRegistry::RegisterLoader(type_hash, [](std::string const& str) -> rp::DescriptorWrapper{											\
			DESC* dptr{ new DESC{rp::serialization::yaml_serializer::deserialize<DESC>(str)} };																		\
			return rp::DescriptorWrapper{ rp::TypeUnsafeTypeErasedWrapper{dptr , [dptr]{delete dptr; } }, type_hash};												\
		});																																							\
		return 1u;																																					\
		}() };																																						\
}

/*delete reinterpret_cast<DESC*>(ptr) potentially undefined behaviour, very dangerous*/

#endif