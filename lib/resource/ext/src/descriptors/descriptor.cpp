#include <descriptors/descriptors.hpp>
#include "descriptors/descriptor_registry.hpp"
#include <chrono>
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assert.h>

namespace {
	constexpr std::uint64_t BITMASK32{(std::uint64_t(0x1)<<32) - 1};
}

namespace Resource {
	std::vector<std::string>& GetTypeVector() {
		static std::vector<std::string> names = { "mesh", "texture", "material", "video", "audio" };
		return names;
	}

	ResourceType GetResourceTypeFromName(std::string const& str) {
		auto& v = GetTypeVector();
		return static_cast<ResourceType>(&(*std::find(v.begin(), v.end(), str)) - &v.front());
	}

	std::string GetResourceTypeName(ResourceType t) {
		return GetTypeVector()[static_cast<std::uint32_t>(t)];
	}

	ResourceDescriptor::ResourceMetadataInfo GetFileMetadata(std::string const& s) {
		ResourceDescriptor::ResourceMetadataInfo meta;
		auto now = std::chrono::steady_clock::now().time_since_epoch();//unix epoch
		meta.m_DateIndexed_ms = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
		std::filesystem::path p{ s };
		meta.m_RawSourcePath = p.lexically_normal().make_preferred().string();
		std::size_t pos = meta.m_RawSourcePath.find(DescriptorRegistry::GetDescriptorRootDirectory());
		pos == std::string::npos ? meta.m_RawSourcePath : meta.m_RawSourcePath = meta.m_RawSourcePath.substr(DescriptorRegistry::GetDescriptorRootDirectory().size());
		meta.m_FileChecksumHash = (static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::filesystem::last_write_time(s).time_since_epoch()).count()) << 32) | (std::filesystem::file_size(s) & BITMASK32);
		return meta;
	}

	ResourceDescriptor ResourceDescriptor::MakeDescriptor(std::string const& s) {
		ResourceDescriptor rdesc{};
		std::size_t extension_pos{ s.find_last_of(".") };
		assert(extension_pos != std::string::npos && extension_pos != s.size() && "unable to deduce importer without file extension");
		std::string file_extension{ s.substr(extension_pos) };
		if (file_extension == ".png" || file_extension == ".jpg" || file_extension == ".jpeg") {
			rdesc = MakeDescriptorTexture(s);
		}
		else if (file_extension == ".obj" || file_extension == ".fbx") {
			rdesc = MakeDescriptorModel(s);
		}
		else {
			//unable to import //unsupported format do not throw
		}
		return rdesc;
	}

	ResourceDescriptor ResourceDescriptor::MakeDescriptorTexture(std::string const& s) {
		ResourceDescriptor rdesc{};
		rdesc.m_RawFileInfo = GetFileMetadata(s);
		ResourceDescriptor::ResourceDescriptorEntry entry{};
		entry.m_Guid = Guid::generate();
		entry.m_Name = rdesc.m_RawFileInfo.m_RawSourcePath;
		rdesc.m_DescriptorEntries.emplace(ResourceType::TEXTURE, entry);
		return rdesc;
	}

	ResourceDescriptor ResourceDescriptor::MakeDescriptorModel(std::string const& s) {
		ResourceDescriptor rdesc{};
		rdesc.m_RawFileInfo = GetFileMetadata(s);
		Assimp::Importer importer;
		const aiScene* scene{ importer.ReadFile(Resource::DescriptorRegistry::GetDescriptorRootDirectory() + rdesc.m_RawFileInfo.m_RawSourcePath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace) };
		for (unsigned int i{}; i < scene->mNumMeshes; i++) {
			ResourceDescriptor::ResourceDescriptorEntry entry{};
			aiMesh* mesh = scene->mMeshes[i];
			entry.m_Guid = Guid::generate();
			entry.m_Name = mesh->mName.C_Str();
			/*aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			for () {
			
			}*/
			rdesc.m_DescriptorEntries.emplace(ResourceType::MESH, entry);
		}
		return rdesc;
	}
}