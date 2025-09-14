#include <locale>
#include <codecvt>
#include <filesystem>
#include <meshoptimizer.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "descriptors/descriptors.h"
#include "compiler/intermediate.h"
#include "texture/texture.h"
#include <yaml-cpp/yaml.h>

namespace Resource {
	std::map<std::string, ResourceDescriptor> virtual_descriptors{};
	std::map<Guid, ResourceDescriptor> guid_descriptors{};
	std::wstring output_dir{std::wstring(std::filesystem::current_path()) + L"/"};
}

namespace {
	auto narrow_to_wstring{
		[](std::string const& str) {
			return std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(str.c_str());
		}
	};
	auto wide_to_string{
		[](std::wstring const& str) {
			return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(str.c_str());
		}
	};
	std::uint32_t assimp_import_flags{ aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType | aiProcess_GenSmoothNormals | aiProcess_FlipUVs };
	constexpr auto reserve_mesh_resource_buffer{
		[](Resource::MeshResource& mref, std::uint32_t vertex_ct, std::uint32_t face_ct, bool has_norms, bool has_colors, bool has_texuvs, bool has_tangs, bool has_bitangs) -> Resource::MeshResource& {
			constexpr std::uint32_t FACE_VERTEX_CT{3};
			mref.m_positions.reserve(vertex_ct);
			mref.m_indices.reserve(face_ct * FACE_VERTEX_CT);
			mref.m_attributes.m_norms.reserve(has_norms ? vertex_ct : 0);
			mref.m_attributes.m_color.reserve(has_colors ? vertex_ct : 0);
			mref.m_attributes.m_uvs.reserve(has_texuvs ? vertex_ct : 0);
			mref.m_attributes.m_tangs.reserve(has_tangs ? vertex_ct : 0);
			mref.m_attributes.m_bitangents.reserve(has_bitangs ? vertex_ct : 0);
			return mref;
		}
	};

	std::wstring get_parent_directory(std::wstring const& path) {
		return path.substr(0,path.find_last_of(L"/"));
	}

	std::string get_parent_directory(std::string const& path) {
		return path.substr(0, path.find_last_of("/"));
	}

	std::wstring get_parent_directory_delimited(std::wstring const& path) {
		return path.substr(0, path.find_last_of(L"/")+1);
	}

	std::string get_parent_directory_delimited(std::string const& path) {
		return path.substr(0, path.find_last_of("/")+1);
	}

	Resource::ResourceDescriptor get_descriptor(std::string const& file_name) {
		Resource::ResourceDescriptor rdesc;
		auto& vdesc{Resource::get_virtual_descriptors()};
		auto& gdesc{ Resource::guid_descriptors };
		if (std::filesystem::exists(file_name)) {
			if (file_name.substr(file_name.find_last_of(".") + 1) == "desc") {
				rdesc = Resource::ResourceDescriptor::load_descriptor(file_name);
				gdesc[rdesc.m_guid] = rdesc;
			}
			else {
				rdesc = Resource::ResourceDescriptor::load_descriptor(file_name.substr(0, file_name.find_last_of(".")) + ".desc");
				gdesc[rdesc.m_guid] = rdesc;
			}
		}
		else if (std::string descfile{ file_name + ".desc" }; std::filesystem::exists(descfile)) {
			rdesc = Resource::ResourceDescriptor::load_descriptor(descfile);
			gdesc[rdesc.m_guid] = rdesc;
		}
		else if (vdesc.find(file_name) != vdesc.end()) {
			rdesc = vdesc[file_name];
			gdesc[rdesc.m_guid] = rdesc;
		}
		else {
			//tmp testing remove this in the future
			//descriptor should exist if not, it is an error. the asset manager should serialise descriptors
			rdesc.m_intermediate_files = narrow_to_wstring(file_name);
			rdesc.m_guid = Resource::Guid::generate();
			vdesc[file_name] = rdesc;
			gdesc[rdesc.m_guid] = rdesc;
		}
		return rdesc;
	};
}

namespace Resource {
	void set_output_dir(std::string const& dir) {
		output_dir = narrow_to_wstring((dir.back() == '/') ? dir : (dir + "/"));
		if (!std::filesystem::exists(output_dir)) {
			std::filesystem::create_directory(output_dir);
		}
	}

	void set_output_dir(std::wstring const& dir) {
		output_dir = dir.back() == '/' ? dir : dir + L"/";
		if (!std::filesystem::exists(output_dir)) {
			std::filesystem::create_directory(output_dir);
		}
	}

	const std::string get_output_dir() {
		return wide_to_string(output_dir);
	}
	std::wstring const& get_output_dir_wstr() {
		return output_dir;
	}

	void generate_material_resource_descriptor_dependencies(const aiScene* aiscene, aiMaterial* aimaterial, ResourceDescriptor& rparent) {
		aiString texPath{ aimaterial->GetName() };
		ResourceDescriptor(get_parent_directory_delimited(rparent.m_intermediate_files) + narrow_to_wstring(texPath.C_Str())).save_descriptor();
		if (aiscene->mNumTextures > 0) {
			assert(0 && "importer does not support embedded textures at the moment");
		}
		else {
			if (aimaterial->GetTextureCount(aiTextureType_DIFFUSE)) {
				if (AI_SUCCESS == aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texPath)) {
					ResourceDescriptor(get_parent_directory_delimited(rparent.m_intermediate_files) + narrow_to_wstring(texPath.C_Str())).save_descriptor();
				}
			}
			if (aimaterial->GetTextureCount(aiTextureType_BASE_COLOR)) {
				if (AI_SUCCESS == aimaterial->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath)) {
					ResourceDescriptor(get_parent_directory_delimited(rparent.m_intermediate_files) + narrow_to_wstring(texPath.C_Str())).save_descriptor();
				}
			}
			if (aimaterial->GetTextureCount(aiTextureType_NORMALS)) {
				if (AI_SUCCESS == aimaterial->GetTexture(aiTextureType_NORMALS, 0, &texPath)) {
					ResourceDescriptor(get_parent_directory_delimited(rparent.m_intermediate_files) + narrow_to_wstring(texPath.C_Str())).save_descriptor();
				}
			}
			if (aimaterial->GetTextureCount(aiTextureType_HEIGHT)) {
				if (AI_SUCCESS == aimaterial->GetTexture(aiTextureType_HEIGHT, 0, &texPath)) {
					ResourceDescriptor(get_parent_directory_delimited(rparent.m_intermediate_files) + narrow_to_wstring(texPath.C_Str())).save_descriptor();
				}
			}
			if (aimaterial->GetTextureCount(aiTextureType_METALNESS)) {
				if (AI_SUCCESS == aimaterial->GetTexture(aiTextureType_METALNESS, 0, &texPath)) {
					ResourceDescriptor(get_parent_directory_delimited(rparent.m_intermediate_files) + narrow_to_wstring(texPath.C_Str())).save_descriptor();
				}
			}
			if (aimaterial->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS)) {
				if (AI_SUCCESS == aimaterial->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texPath)) {
					ResourceDescriptor(get_parent_directory_delimited(rparent.m_intermediate_files) + narrow_to_wstring(texPath.C_Str())).save_descriptor();
				}
			}
			if (aimaterial->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION)) {
				if (AI_SUCCESS == aimaterial->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &texPath)) {
					ResourceDescriptor(get_parent_directory_delimited(rparent.m_intermediate_files) + narrow_to_wstring(texPath.C_Str())).save_descriptor();
				}
			}
			if (aimaterial->GetTextureCount(aiTextureType_EMISSIVE)) {
				if (AI_SUCCESS == aimaterial->GetTexture(aiTextureType_EMISSIVE, 0, &texPath)) {
					ResourceDescriptor(get_parent_directory_delimited(rparent.m_intermediate_files) + narrow_to_wstring(texPath.C_Str())).save_descriptor();
				}
			}

		}
	}

	void generate_model_resource_descriptor_dependencies(ResourceDescriptor& rdesc) {
		Assimp::Importer importer{};
		std::string narrow_name{ std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(rdesc.m_intermediate_files) };
		const aiScene* fscene{ importer.ReadFile(narrow_name, assimp_import_flags) };
		assert(fscene && "file error");
		for (std::uint32_t i{}; i < fscene->mNumMaterials; i++) {
			aiMaterial* aimaterial{ fscene->mMaterials[i] };
			generate_material_resource_descriptor_dependencies(fscene, aimaterial, rdesc);
		}
		for (std::uint32_t i{}; i < fscene->mNumMeshes; i++) {
			aiMesh* aimesh{ fscene->mMeshes[i] };
			ResourceDescriptor(get_parent_directory_delimited(rdesc.m_intermediate_files) + narrow_to_wstring(aimesh->mName.C_Str())).save_descriptor();
		}
	}

	void ResourceDescriptor::generate_resource_descriptor_dependencies() {
		if (m_resource_type == ResourceType::MODEL) {
			generate_model_resource_descriptor_dependencies(*this);
		}
	}

	void compile_geometry(ResourceDescriptor const& rdesc) {
		auto mesh{ load_mesh(std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(rdesc.m_intermediate_files)) };
		std::ofstream outp(get_output_dir() + rdesc.m_guid.to_hex_no_delimiter() + ".mesh", std::ios::binary);
		mesh >> outp;
	}

	void compile_model(ResourceDescriptor const& rdesc) {
		auto& desc{ get_virtual_descriptors() };
		auto& gdesc{ Resource::guid_descriptors };
		std::string narrow_name{ std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(rdesc.m_intermediate_files) };
		if (desc.find(narrow_name) == desc.end()) {
			desc[narrow_name] = rdesc;
			gdesc[rdesc.m_guid] = rdesc;
		}
		auto model{ load_model(narrow_name) };
		std::ofstream outp(get_output_dir() + rdesc.m_guid.to_hex_no_delimiter() + ".model", std::ios::binary);
		model >> outp;
		for (auto const& mesh : model.m_meshes) {
			std::ofstream mesh_outp(get_output_dir() + mesh.m_guid.to_hex_no_delimiter() + ".mesh", std::ios::binary);
			mesh >> mesh_outp;
			mesh_outp.close();
		}
		for (auto const& mat : model.m_mats) {
			std::ofstream mat_outp(get_output_dir() + mat.m_guid.to_hex_no_delimiter() + ".material", std::ios::binary);
			mat >> mat_outp;
			mat_outp.close();
			
			if (mat.m_color_tex) {
				//gdesc[mat.m_color_tex].m_intermediate_files = get_parent_directory_delimited(rdesc.m_intermediate_files) + gdesc[mat.m_color_tex].m_intermediate_files;
				compile_texture(gdesc[mat.m_color_tex]);
			}
			if (mat.m_diffuse_tex) {
				//gdesc[mat.m_diffuse_tex].m_intermediate_files = get_parent_directory_delimited(rdesc.m_intermediate_files) + gdesc[mat.m_diffuse_tex].m_intermediate_files;
				compile_texture(gdesc[mat.m_diffuse_tex]);
			}
			if (mat.m_emissive_tex) {
				//gdesc[mat.m_emissive_tex].m_intermediate_files = get_parent_directory_delimited(rdesc.m_intermediate_files) + gdesc[mat.m_emissive_tex].m_intermediate_files;
				compile_texture(gdesc[mat.m_emissive_tex]);
			}
			if (mat.m_height_tex) {
				//gdesc[mat.m_height_tex].m_intermediate_files = get_parent_directory_delimited(rdesc.m_intermediate_files) + gdesc[mat.m_height_tex].m_intermediate_files;
				compile_texture(gdesc[mat.m_height_tex]);
			}
			if (mat.m_metallic_tex) {
				//gdesc[mat.m_metallic_tex].m_intermediate_files = get_parent_directory_delimited(rdesc.m_intermediate_files) + gdesc[mat.m_metallic_tex].m_intermediate_files;
				compile_texture(gdesc[mat.m_metallic_tex]);
			}
			if (mat.m_normal_tex) {
				//gdesc[mat.m_normal_tex].m_intermediate_files = get_parent_directory_delimited(rdesc.m_intermediate_files) + gdesc[mat.m_normal_tex].m_intermediate_files;
				compile_texture(gdesc[mat.m_normal_tex]);
			}
			if (mat.m_occlusion_tex) {
				//gdesc[mat.m_occlusion_tex].m_intermediate_files = get_parent_directory_delimited(rdesc.m_intermediate_files) + gdesc[mat.m_occlusion_tex].m_intermediate_files;
				compile_texture(gdesc[mat.m_occlusion_tex]);
			}
			if (mat.m_roughness_tex) {
				//gdesc[mat.m_roughness_tex].m_intermediate_files = get_parent_directory_delimited(rdesc.m_intermediate_files) + gdesc[mat.m_roughness_tex].m_intermediate_files;
				compile_texture(gdesc[mat.m_roughness_tex]);
			}
		}
		outp.close();
	}

	void compile_descriptor(ResourceDescriptor const& rdesc) {
		switch (rdesc.m_resource_type) {
		case ResourceType::MODEL:
			compile_model(rdesc);
			break;
		case ResourceType::MESH:
			compile_geometry(rdesc);
			break;
		case ResourceType::TEXTURE:
			compile_texture(rdesc);
			break;
		}
	}

	std::map<std::string, Resource::ResourceDescriptor>& get_virtual_descriptors() {
		return Resource::virtual_descriptors;
	}

	MeshResource load_mesh(aiMesh* aimesh, std::string path = {}) {
		constexpr std::uint32_t BONE_VERTEX_CT{ 4 };
		MeshResource mres;
		mres.m_guid = get_descriptor(path + aimesh->mName.C_Str()).m_guid;
		mres.m_properties.m_mat_idx = aimesh->mMaterialIndex;
		reserve_mesh_resource_buffer(mres, aimesh->mNumVertices, aimesh->mNumFaces, aimesh->HasNormals(), aimesh->HasVertexColors(0), aimesh->HasTextureCoords(0), aimesh->HasTangentsAndBitangents(), aimesh->HasTangentsAndBitangents());
		for (std::uint32_t j{}; j < aimesh->mNumVertices; j++) {
			static_assert(sizeof(glm::vec3) == sizeof(aiVector3D));
			mres.m_positions.emplace_back(*reinterpret_cast<glm::vec3*>(&aimesh->mVertices[j]));
			if (aimesh->HasNormals()) {
				mres.m_attributes.m_norms.emplace_back(*reinterpret_cast<glm::vec3*>(&aimesh->mNormals[j]));
			}
			if (aimesh->HasTangentsAndBitangents()) {
				mres.m_attributes.m_tangs.emplace_back(*reinterpret_cast<glm::vec3*>(&aimesh->mTangents[j]));
				mres.m_attributes.m_bitangents.emplace_back(*reinterpret_cast<glm::vec3*>(&aimesh->mBitangents[j]));
			}
			if (aimesh->HasTextureCoords(j)) {
				mres.m_attributes.m_uvs.emplace_back(*reinterpret_cast<glm::vec2*>(&aimesh->mTextureCoords[j]));
			}
			if (aimesh->HasVertexColors(j)) {
				mres.m_attributes.m_color.emplace_back(*reinterpret_cast<glm::vec4*>(&aimesh->mColors[j]));
			}
		}
		for (std::uint32_t j{}; j < aimesh->mNumFaces; j++) {
			aiFace aiface{ aimesh->mFaces[j] };
			mres.m_indices.insert(mres.m_indices.end(), aiface.mIndices, aiface.mIndices + aiface.mNumIndices);
		}
		mres.m_attributes.m_bone_indices.resize(aimesh->HasBones() ? aimesh->mNumVertices * BONE_VERTEX_CT : 0);
		mres.m_attributes.m_bone_influence.resize(aimesh->HasBones() ? aimesh->mNumVertices * BONE_VERTEX_CT : 0);
		for (std::uint32_t i{}; i < aimesh->mNumBones; i++) {
			const aiBone* aibone{ aimesh->mBones[i]};
			for (std::uint32_t j{}; j < aibone->mNumWeights; j++) {
				std::uint32_t vertex_id = aibone->mWeights[j].mVertexId;
				float weight = aibone->mWeights[j].mWeight;

				uint32_t* bone_index_ptr = &mres.m_attributes.m_bone_indices[vertex_id * BONE_VERTEX_CT];
				float* bone_influence_ptr = &mres.m_attributes.m_bone_influence[vertex_id * BONE_VERTEX_CT];
				for (int k = 0; k < BONE_VERTEX_CT; k++) {
					if (bone_influence_ptr[k] == 0.0f) {
						bone_index_ptr[k] = i;
						bone_influence_ptr[k] = weight;
						break;
					}
				}
			}
		}

		return mres;
	}

	MaterialResource load_material(const aiScene* aiscene, const aiMaterial* aimaterial, std::string const& path = {}) {
		MaterialResource matres;
		aiString name;
		aimaterial->Get(AI_MATKEY_NAME, name);
		matres.m_guid = get_descriptor(path + name.C_Str()).m_guid;
		aiColor4D diff; 
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diff)) {
			matres.m_color = { diff.r,diff.g,diff.b };
		}
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_COLOR_SPECULAR, diff)) {
			matres.m_specular_color = { diff.r,diff.g,diff.b };
		}
		if (AI_FAILURE == aimaterial->Get(AI_MATKEY_OPACITY, matres.m_opacity)) {
			matres.m_opacity = 1.f; //default value for opacity
		}
		aimaterial->Get(AI_MATKEY_SHININESS, matres.m_shininess);
		if (AI_FAILURE == aimaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, matres.m_roughness)) {
			matres.m_roughness = 1.0f / (1.0f + matres.m_shininess); //fallback
		}

		if (aiscene->mNumTextures > 0) {
			assert(0 && "importer does not support embedded textures at the moment");
		}
		else {
			aiString texPath;
			if (aimaterial->GetTextureCount(aiTextureType_DIFFUSE)) {
				if (AI_SUCCESS == aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texPath)) {
					matres.m_diffuse_tex = get_descriptor(path + texPath.C_Str()).m_guid;
				}
			}
			if (aimaterial->GetTextureCount(aiTextureType_BASE_COLOR)) {
				if (AI_SUCCESS == aimaterial->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath)) {
					matres.m_color_tex = get_descriptor(path + texPath.C_Str()).m_guid;
				}
			}
			if (aimaterial->GetTextureCount(aiTextureType_NORMALS)) {
				if (AI_SUCCESS == aimaterial->GetTexture(aiTextureType_NORMALS, 0, &texPath)) {
					matres.m_normal_tex = get_descriptor(path + texPath.C_Str()).m_guid;
				}
			}
			if (aimaterial->GetTextureCount(aiTextureType_HEIGHT)) {
				if (AI_SUCCESS == aimaterial->GetTexture(aiTextureType_HEIGHT, 0, &texPath)) {
					matres.m_height_tex = get_descriptor(path + texPath.C_Str()).m_guid;
				}
			}
			if (aimaterial->GetTextureCount(aiTextureType_METALNESS)) {
				if (AI_SUCCESS == aimaterial->GetTexture(aiTextureType_METALNESS, 0, &texPath)) {
					matres.m_metallic_tex = get_descriptor(path + texPath.C_Str()).m_guid;
				}
			}
			if (aimaterial->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS)) {
				if (AI_SUCCESS == aimaterial->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texPath)) {
					matres.m_roughness_tex = get_descriptor(path + texPath.C_Str()).m_guid;
				}
			}
			if (aimaterial->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION)) {
				if (AI_SUCCESS == aimaterial->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &texPath)) {
					matres.m_occlusion_tex = get_descriptor(path + texPath.C_Str()).m_guid;
				}
			}
			if (aimaterial->GetTextureCount(aiTextureType_EMISSIVE)) {
				if (AI_SUCCESS == aimaterial->GetTexture(aiTextureType_EMISSIVE, 0, &texPath)) {
					matres.m_emissive_tex = get_descriptor(path + texPath.C_Str()).m_guid;
				}
			}

		}

		return matres;
	}

	SkeletalResource load_skeleton(const aiScene* aiscene) {
		SkeletalResource skelres;
		//todo
		return skelres;
	}

	ModelResource load_model(std::string const& file_path) {
		Assimp::Importer importer{};
		const aiScene* fscene{ importer.ReadFile(file_path, assimp_import_flags) };
		ModelResource mdlres;
		assert(fscene && "file error");
		std::string parent_dir{ get_parent_directory_delimited(file_path) };

		//mdlres.m_skel = load_skeleton(fscene);

		for (std::uint32_t i{}; i < fscene->mNumMaterials; i++) {
			aiMaterial* aimaterial{ fscene->mMaterials[i] };
			mdlres.m_mats.emplace_back(load_material(fscene, aimaterial,parent_dir));
		}

		for (std::uint32_t i{}; i < fscene->mNumMeshes; i++) {
			aiMesh* aimesh{ fscene->mMeshes[i] };
			mdlres.m_meshes.emplace_back(load_mesh(aimesh, parent_dir));
		}

		return mdlres;
	}

	MeshResource load_mesh(std::string const& file_path) {
		Assimp::Importer importer{};
		const aiScene* fscene{ importer.ReadFile(file_path, assimp_import_flags) };
		MeshResource mres;
		assert(fscene && "file error");

		for (std::uint32_t i{}; i < fscene->mNumMeshes; i++) {
			aiMesh* aimesh{ fscene->mMeshes[i] };
			mres.m_properties.m_mat_idx = aimesh->mMaterialIndex;
			for (std::uint32_t j{}; j < aimesh->mNumVertices; j++) {
				static_assert(sizeof(glm::vec3)==sizeof(aiVector3D));
				mres.m_positions.emplace_back(*reinterpret_cast<glm::vec3*>(&aimesh->mVertices[j]));
				if (aimesh->HasNormals()) {
					mres.m_attributes.m_norms.emplace_back(*reinterpret_cast<glm::vec3*>(&aimesh->mNormals[j]));
				}
				if (aimesh->HasTextureCoords(j)) {
					mres.m_attributes.m_uvs.emplace_back(*reinterpret_cast<glm::vec2*>(&aimesh->mTextureCoords[j]));
					mres.m_attributes.m_tangs.emplace_back(*reinterpret_cast<glm::vec3*>(&aimesh->mTangents[j]));
					mres.m_attributes.m_bitangents.emplace_back(*reinterpret_cast<glm::vec3*>(&aimesh->mBitangents[j]));
				}
				if (aimesh->HasVertexColors(j)) {
					mres.m_attributes.m_color.emplace_back(*reinterpret_cast<glm::vec4*>(&aimesh->mColors[j]));
				}
			}
			for (std::uint32_t j{}; j < aimesh->mNumFaces; j++) {
				aiFace aiface{ aimesh->mFaces[j]};
				mres.m_indices.insert(mres.m_indices.end(), aiface.mIndices, aiface.mIndices + aiface.mNumIndices);
			}
		}
		return mres;
	}
}