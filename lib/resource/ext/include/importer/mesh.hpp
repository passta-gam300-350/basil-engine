#ifndef RESOURCE_IMPORTOR_MESH
#define RESOURCE_IMPORTOR_MESH

#define GLM_ENABLE_EXPERIMENTAL

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "descriptors/mesh.hpp"
#include "descriptors/material.hpp"
#include "descriptors/texture.hpp"
#include <native/mesh.h>

// --- helper: build transform matrix ---
inline glm::mat4 BuildTransform(ModelDescriptor const& desc) {
    glm::mat4 T(1.0f);
    T = glm::translate(T, desc.translate);
    T = T * glm::yawPitchRoll(
        glm::radians(desc.rotate.y),
        glm::radians(desc.rotate.x),
        glm::radians(desc.rotate.z)
    );
    T = glm::scale(T, desc.scale);
    return T;
}

// --- helper: convert aiColor3D to glm::vec3 ---
inline glm::vec3 ToVec3(const aiColor3D& c) {
    return glm::vec3(c.r, c.g, c.b);
}

// --- helper: extract material into MaterialDescriptor ---
inline MaterialDescriptor ExtractMaterial(aiMaterial* aimat) {
    MaterialDescriptor matDesc{};
    aiString name;
    if (AI_SUCCESS == aimat->Get(AI_MATKEY_NAME, name)) {
        matDesc.material.material_name = name.C_Str();
    }

    // Albedo / diffuse
    aiColor3D color(0.f, 0.f, 0.f);
    if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
        matDesc.material.albedo = ToVec3(color);
    }

    // Roughness / shininess
    float shininess = 0.f;
    if (AI_SUCCESS == aimat->Get(AI_MATKEY_SHININESS, shininess)) {
        // crude mapping: roughness = 1 - glossiness
        matDesc.material.roughness = glm::clamp(1.0f - shininess / 128.0f, 0.0f, 1.0f);
    }

    // Metallic (not always present in FBX/OBJ, but glTF may have it)
    float metallic = 0.f;
    if (AI_SUCCESS == aimat->Get(AI_MATKEY_METALLIC_FACTOR, metallic)) {
        matDesc.material.metallic = metallic;
    }

    const auto getTextureGuid{ [](std::string const& texname) {
        if (std::string desc_path{texname.substr(0, texname.rfind('.')) + ".desc"};std::filesystem::exists(desc_path)) {
            return rp::serialization::yaml_serializer::deserialize<TextureDescriptor>(desc_path).base.m_guid;
        }
        else {
            return rp::null_guid;
        }
        } };
    // Textures
    aiString texPath;
    if (AI_SUCCESS == aimat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath)) {
        matDesc.material.texture_properties["albedo"] = getTextureGuid(texPath.C_Str());
    }
    if (AI_SUCCESS == aimat->GetTexture(aiTextureType_NORMALS, 0, &texPath)) {
        matDesc.material.texture_properties["normal"] = getTextureGuid(texPath.C_Str());
    }
    if (AI_SUCCESS == aimat->GetTexture(aiTextureType_METALNESS, 0, &texPath)) {
        matDesc.material.texture_properties["metallic"] = getTextureGuid(texPath.C_Str());
    }
    if (AI_SUCCESS == aimat->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texPath)) {
        matDesc.material.texture_properties["roughness"] = getTextureGuid(texPath.C_Str());
    }

    matDesc.material.vert_name = "pbr.vert";
    matDesc.material.frag_name = "pbr.frag";

    // Assign a new Guid for this material
    matDesc.base.m_guid = rp::Guid::generate();
    matDesc.base.m_importer = "material";

    return matDesc;
}

// --- helper: process one aiMesh ---
inline MeshResourceData::Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene, glm::mat4 const& transform, bool extract_material, std::vector<MaterialDescriptor>& outMaterials)
{
    MeshResourceData::Mesh out;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        MeshResourceData::Vertex v{};

        glm::vec4 pos(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
        pos = transform * pos;
        v.Position = glm::vec3(pos);

        if (mesh->HasNormals()) {
            glm::vec4 n(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z, 0.0f);
            n = transform * n;
            v.Normal = glm::normalize(glm::vec3(n));
        }

        if (mesh->mTextureCoords[0]) {
            v.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        else {
            v.TexCoords = glm::vec2(0.0f);
        }

        if (mesh->HasTangentsAndBitangents()) {
            glm::vec4 t(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z, 0.0f);
            glm::vec4 b(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z, 0.0f);
            t = transform * t;
            b = transform * b;
            v.Tangent = glm::normalize(glm::vec3(t));
            v.Bitangent = glm::normalize(glm::vec3(b));
        }

        for (int j = 0; j < MAX_BONE_INFLUENCE; j++) {
            v.m_BoneIDs[j] = -1;
            v.m_Weights[j] = 0.0f;
        }

        out.vertices.push_back(v);
    }

    // indices
    for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
        aiFace face = mesh->mFaces[f];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            out.indices.push_back(face.mIndices[j]);
        }
    }

    // material slot
    MeshResourceData::MaterialSlot slot{};
    slot.index_begin = 0;
    slot.index_count = static_cast<unsigned int>(out.indices.size());

    if (extract_material) {
        unsigned int matIndex = mesh->mMaterialIndex;
        if (matIndex < scene->mNumMaterials) {
            aiMaterial* aimat = scene->mMaterials[matIndex];
            MaterialDescriptor matDesc = ExtractMaterial(aimat);
            slot.material_guid = matDesc.base.m_guid;
            outMaterials.push_back(std::move(matDesc));
        }
    }

    out.materials.push_back(slot);
    return out;
}

// --- main ImportModel ---
inline std::vector<std::pair<rp::Guid, MeshResourceData>> ImportModel(ModelDescriptor const& desc) {
    std::vector<std::pair<rp::Guid, MeshResourceData>> result;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        rp::utility::resolve_path(desc.base.m_source),
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices
    );

    if (!scene || !scene->HasMeshes()) {
        throw std::runtime_error("Failed to load model: " + desc.base.m_source);
    }

    glm::mat4 transform = BuildTransform(desc);

    // Collect all extracted materials
    std::vector<MaterialDescriptor> extractedMaterials;

    if (desc.merge_mesh) {
        MeshResourceData merged;
        MeshResourceData::Mesh mergedMesh;
        unsigned int indexOffset = 0;

        for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
            MeshResourceData::Mesh mesh = ProcessMesh(scene->mMeshes[m], scene, transform,
                desc.extract_material, extractedMaterials);

            // append vertices
            mergedMesh.vertices.insert(mergedMesh.vertices.end(),
                mesh.vertices.begin(),
                mesh.vertices.end());

            unsigned int index_size{ static_cast<unsigned int>(mergedMesh.indices.size()) };

            // append indices with offset
            for (auto idx : mesh.indices) {
                mergedMesh.indices.push_back(idx + indexOffset);
            }

            // adjust material slot
            for (auto& slot : mesh.materials) {
                MeshResourceData::MaterialSlot newSlot = slot;
                newSlot.index_begin = index_size;
                mergedMesh.materials.push_back(newSlot);
            }
            indexOffset += static_cast<unsigned int>(mesh.vertices.size());
        }

        merged.meshes.push_back(std::move(mergedMesh));
        result.emplace_back(std::pair<rp::Guid, MeshResourceData>(desc.base.m_guid, std::move(merged)));
    }
    else {
        for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
            MeshResourceData res;
            res.meshes.push_back(ProcessMesh(scene->mMeshes[m], scene, transform,
                desc.extract_material, extractedMaterials));
            result.emplace_back(std::pair<rp::Guid, MeshResourceData>(rp::Guid::generate(), std::move(res)));
        }
    }

    // At this point, extractedMaterials contains all material descriptors
    // You can now call CreateMaterial(matDesc) for each to register them in your engine.
    for (auto const& matDesc : extractedMaterials) {
        CreateMaterial(matDesc, rp::utility::output_path());
    }

    return result;
}

RegisterResourceTypeImporter(ModelDescriptor, MeshResourceData, "mesh", ".mesh", ImportModel, ".gltf", ".obj", ".fbx")

#endif