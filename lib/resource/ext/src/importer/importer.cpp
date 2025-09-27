#include "importer/importer.hpp"
#include "importer/importer_registry.hpp"
#include "native/loader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <ranges>

#include <string>
#include <locale>
#include <codecvt>

#include <filesystem>

namespace {
	std::wstring string_to_wstring(const std::string& str) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
		return conv.from_bytes(str);
	}

	std::string wstring_to_string(const std::wstring& wstr) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
		return conv.to_bytes(wstr);
	}
}

namespace Resource {
	//general import, attempts to import based on extension
	void Import(ResourceDescriptor& rdesc) {
		std::size_t extension_pos{ rdesc.m_RawFileInfo.m_RawSourcePath.find_last_of(".") };
		assert(extension_pos != std::string::npos && extension_pos != rdesc.m_RawFileInfo.m_RawSourcePath.size() && "unable to deduce importer without file extension");
		std::string file_extension{ rdesc.m_RawFileInfo.m_RawSourcePath.substr(extension_pos) };
		if (file_extension == ".png" || file_extension == ".jpg" || file_extension == ".jpeg") {
			ImportTexture(rdesc);
		}
		else if (file_extension == ".obj" || file_extension == ".fbx") {
			ImportModel(rdesc);
		}
		else {
			//unable to import //unsupported format do not throw
		}
	}

	int get_material_texture_guid(aiMaterial* mat, TextureType texturetype, std::vector<Resource::Guid>& handles) {
		int count{};
		auto type{ aiTextureType_SPECULAR };
		switch (texturetype) {
		case TextureType::SPECULAR_BITMAP:
			type = aiTextureType_SPECULAR;
			for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
			{
				aiString str;
				mat->GetTexture(type, i, &str);
				std::string stdstr{ str.C_Str() };
				std::transform(stdstr.begin(), stdstr.end(), stdstr.begin(), [](unsigned char c) { return std::tolower(c); });
				if (stdstr.find("metallic") == std::string::npos || stdstr.find("roughness") == std::string::npos) {
					continue;
				}
				if (std::string fstr{ str.C_Str() }, dstr{ fstr.substr(0, fstr.find_last_of(".")) + ".desc" }; std::filesystem::exists(fstr) && std::filesystem::exists(dstr)) {
					handles.emplace_back(ResourceDescriptor::LoadDescriptor(fstr).m_DescriptorEntries.begin()->second.m_Guid);
					count++;
				}
				count++;
			}
			break;
		case TextureType::METALLIC_BITMAP:
			type = aiTextureType_SPECULAR;
			for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
			{
				aiString str;
				mat->GetTexture(type, i, &str);
				std::string stdstr{ str.C_Str() };
				std::transform(stdstr.begin(), stdstr.end(), stdstr.begin(), [](unsigned char c) { return std::tolower(c); });
				if (stdstr.find("metallic") != std::string::npos) {
					if (std::string fstr{ str.C_Str() }, dstr{ fstr.substr(0, fstr.find_last_of(".")) + ".desc" }; std::filesystem::exists(fstr) && std::filesystem::exists(dstr)) {
						handles.emplace_back(ResourceDescriptor::LoadDescriptor(fstr).m_DescriptorEntries.begin()->second.m_Guid);
						count++;
					}
				}
			}
			break;
		case TextureType::ROUGHNESS_BITMAP:
			type = aiTextureType_SPECULAR;
			for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
			{
				aiString str;
				mat->GetTexture(type, i, &str);
				std::string stdstr{ str.C_Str() };
				std::transform(stdstr.begin(), stdstr.end(), stdstr.begin(), [](unsigned char c) { return std::tolower(c); });
				if (stdstr.find("roughness") != std::string::npos) {
					if (std::string fstr{ str.C_Str() }, dstr{ fstr.substr(0, fstr.find_last_of(".")) + ".desc" }; std::filesystem::exists(fstr) && std::filesystem::exists(dstr)) {
						handles.emplace_back(ResourceDescriptor::LoadDescriptor(fstr).m_DescriptorEntries.begin()->second.m_Guid);
						count++;
					}
				}
			}
			type = aiTextureType_SHININESS;
			for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
			{
				aiString str;
				mat->GetTexture(type, i, &str);
				if (std::string fstr{ str.C_Str() }, dstr{ fstr.substr(0, fstr.find_last_of(".")) + ".desc" }; std::filesystem::exists(fstr) && std::filesystem::exists(dstr)) {
					handles.emplace_back(ResourceDescriptor::LoadDescriptor(fstr).m_DescriptorEntries.begin()->second.m_Guid);
					count++;
				}
			}
			break;
			break;
		case TextureType::HEIGHT_BITMAP:
			type = aiTextureType_HEIGHT;
			for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
			{
				aiString str;
				mat->GetTexture(type, i, &str);
				std::string stdstr{ str.C_Str() };
				std::transform(stdstr.begin(), stdstr.end(), stdstr.begin(), [](unsigned char c) { return std::tolower(c); });
				if (stdstr.find("normal") == std::string::npos) {
					if (std::string fstr{ str.C_Str() }, dstr{ fstr.substr(0, fstr.find_last_of(".")) + ".desc" }; std::filesystem::exists(fstr) && std::filesystem::exists(dstr)) {
						handles.emplace_back(ResourceDescriptor::LoadDescriptor(fstr).m_DescriptorEntries.begin()->second.m_Guid);
						count++;
					}
				}
			}
			type = aiTextureType_DISPLACEMENT;
			for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
			{
				aiString str;
				mat->GetTexture(type, i, &str);
				std::string stdstr{ str.C_Str() };
				std::transform(stdstr.begin(), stdstr.end(), stdstr.begin(), [](unsigned char c) { return std::tolower(c); });
				if (std::string fstr{ str.C_Str() }, dstr{ fstr.substr(0, fstr.find_last_of(".")) + ".desc" }; std::filesystem::exists(fstr) && std::filesystem::exists(dstr)) {
					handles.emplace_back(ResourceDescriptor::LoadDescriptor(fstr).m_DescriptorEntries.begin()->second.m_Guid);
					count++;
				}
			}
			break;
		case TextureType::NORMAL_BITMAP:
			type = aiTextureType_NORMALS;
			for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
			{
				aiString str;
				mat->GetTexture(type, i, &str);
				if (std::string fstr{ str.C_Str() }, dstr{ fstr.substr(0, fstr.find_last_of(".")) + ".desc" }; std::filesystem::exists(fstr) && std::filesystem::exists(dstr)) {
					handles.emplace_back(ResourceDescriptor::LoadDescriptor(fstr).m_DescriptorEntries.begin()->second.m_Guid);
					count++;
				}
			}
			type = aiTextureType_HEIGHT;
			for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
			{
				aiString str;
				mat->GetTexture(type, i, &str);
				std::string stdstr{ str.C_Str() };
				std::transform(stdstr.begin(), stdstr.end(), stdstr.begin(), [](unsigned char c) { return std::tolower(c); });
				if (stdstr.find("normal") != std::string::npos) {
					if (std::string fstr{ str.C_Str() }, dstr{ fstr.substr(0, fstr.find_last_of(".")) + ".desc"}; std::filesystem::exists(fstr) && std::filesystem::exists(dstr)) {
						handles.emplace_back(ResourceDescriptor::LoadDescriptor(fstr).m_DescriptorEntries.begin()->second.m_Guid);
						count++;
					}
				}
			}
			break;
		case TextureType::DIFFUSE_BITMAP:
		default:
			type = aiTextureType_DIFFUSE;
			for (unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_DIFFUSE); i++)
			{
				aiString str;
				mat->GetTexture(type, i, &str);
				if (std::string fstr{ str.C_Str() }, dstr{ fstr.substr(0, fstr.find_last_of(".")) + ".desc" }; std::filesystem::exists(fstr) && std::filesystem::exists(dstr)) {
					handles.emplace_back(ResourceDescriptor::LoadDescriptor(fstr).m_DescriptorEntries.begin()->second.m_Guid);
					count++;
				}
			}
			break;
		}
		return count;
	}

	MeshAssetData GetNativeMesh(aiMesh* aimesh, aiMaterial* aimaterial) {
		constexpr std::uint32_t BONE_VERTEX_CT{ 4 };
		constexpr aiTextureType texture_type_list[] = { aiTextureType_DIFFUSE, };
		MeshAssetData m_data;
		m_data.vertices.reserve(aimesh->mNumVertices);
		m_data.indices.reserve(aimesh->mNumFaces * 3);
		for (std::uint32_t j{}; j < aimesh->mNumVertices; j++) {
			static_assert(sizeof(glm::vec3) == sizeof(aiVector3D));
			MeshAssetData::Vertex vert{};
			vert.Position = *reinterpret_cast<glm::vec3*>(&aimesh->mVertices[j]);
			if (aimesh->HasNormals()) {
				vert.Normal = *reinterpret_cast<glm::vec3*>(&aimesh->mNormals[j]);
			}
			if (aimesh->HasTangentsAndBitangents()) {
				vert.Tangent = *reinterpret_cast<glm::vec3*>(&aimesh->mTangents[j]);
				vert.Bitangent = *reinterpret_cast<glm::vec3*>(&aimesh->mBitangents[j]);
			}
			if (aimesh->HasTextureCoords(j)) {
				vert.TexCoords = *reinterpret_cast<glm::vec2*>(&aimesh->mTextureCoords[j]);
			}
		}
		for (std::uint32_t j{}; j < aimesh->mNumFaces; j++) {
			aiFace aiface{ aimesh->mFaces[j] };
			m_data.indices.insert(m_data.indices.end(), aiface.mIndices, aiface.mIndices + aiface.mNumIndices);
		}
		for (std::uint32_t i{}; i < aimesh->mNumBones; i++) {
			const aiBone* aibone{ aimesh->mBones[i] };
			for (std::uint32_t j{}; j < aibone->mNumWeights; j++) {
				std::uint32_t vertex_id = aibone->mWeights[j].mVertexId;
				float weight = aibone->mWeights[j].mWeight;

				int* bone_index_ptr = m_data.vertices[vertex_id].m_BoneIDs;
				float* bone_influence_ptr = m_data.vertices[vertex_id].m_Weights;
				for (int k = 0; k < BONE_VERTEX_CT; k++) {
					if (bone_influence_ptr[k] == 0.0f) {
						bone_index_ptr[k] = i;
						bone_influence_ptr[k] = weight;
						break;
					}
				}
			}
		}

		for (std::uint8_t i{}; i < static_cast<std::uint8_t>(TextureType::MAX_COUNT); i++) {
			TextureType type = static_cast<TextureType>(i);
			int count = get_material_texture_guid(aimaterial, type, m_data.textures);
			m_data.texture_type.insert(m_data.texture_type.end(), count, static_cast<std::byte>(type));
		}

		return m_data;
	}

	void ImportModel(ResourceDescriptor& rdesc) {
		Assimp::Importer importer{};
		const aiScene* scene{ importer.ReadFile(rdesc.m_RawFileInfo.m_RawSourcePath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace) };
		auto [mesh_begin, mesh_end] {rdesc.m_DescriptorEntries.equal_range(ResourceType::MESH) };
		auto mesh_range{std::ranges::subrange(mesh_begin, mesh_end)};
		for (unsigned int i{}; i < scene->mNumMeshes; i++) {
			if (auto mesh_descriptor{ std::ranges::find_if(mesh_range, [&](auto const& entry) {return entry.second.m_Name == scene->mMeshes[i]->mName.C_Str();}) }; 
					mesh_descriptor != mesh_end) {
				auto native_data{ GetNativeMesh(scene->mMeshes[i], scene->mMaterials[scene->mMeshes[i]->mMaterialIndex]) };
				std::ofstream ofs(ImporterRegistry::GetImportDirectory() + "/" + mesh_descriptor->second.m_Guid.to_hex_no_delimiter() + ".mesh", std::ios::binary);
				native_data >> ofs;
				ofs.close();
			}
		}
	}

	void ImportTexture(ResourceDescriptor& rdesc) {
		CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		DirectX::ScratchImage image;
		DirectX::ScratchImage cimage;
		DirectX::TexMetadata info;

		auto hres = LoadFromWICFile(string_to_wstring(rdesc.m_RawFileInfo.m_RawSourcePath).c_str(), DirectX::WIC_FLAGS::WIC_FLAGS_NONE, &info, image);
		assert(!FAILED(hres) && "load fail");
		hres = Compress(image.GetImages(), image.GetImageCount(), info, DXGI_FORMAT_BC7_UNORM, DirectX::TEX_COMPRESS_PARALLEL | DirectX::TEX_COMPRESS_BC7_QUICK, DirectX::TEX_THRESHOLD_DEFAULT, cimage);
		assert(!FAILED(hres) && "compress fail");
		TextureAsset tasset{};
		hres = DirectX::SaveToDDSMemory(cimage.GetImages(), cimage.GetImageCount(), cimage.GetMetadata(), DirectX::DDS_FLAGS_NONE, tasset.m_TexData);
		assert(!FAILED(hres) && "save fail");
		std::ofstream ofs(ImporterRegistry::GetImportDirectory() + "/" + rdesc.m_DescriptorEntries.find(ResourceType::TEXTURE)->second.m_Guid.to_hex_no_delimiter() + ".texture", std::ios::binary);
		tasset >> ofs;
	}
};