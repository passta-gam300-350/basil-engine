#ifndef RESOURCE_IMPORTOR_MESH
#define RESOURCE_IMPORTOR_MESH

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "descriptors/mesh.hpp"
#include "descriptors/material.hpp"
#include "descriptors/texture.hpp"
#include "descriptors/animation.hpp"
#include "descriptors/skeleton.hpp"
#include <native/mesh.h>

#include <stb_image.h>
#include <meshoptimizer.h>

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

// --- helper: convert aiVector3D to glm::vec3 ---
inline glm::vec3 ToVec3(const aiVector3D& c) {
    return glm::vec3(c.x, c.y, c.z);
}

// --- helper: convert aiquat to glm::vec3 ---
inline glm::quat ToQuat(const aiQuaternion& q) {
    //return glm::quat(q.x, q.y, q.z, q.w);
    return glm::quat(q.w, q.x, q.y, q.z);
}

// --- helper: convert aiquat to glm::vec3 ---
//inline glm::mat4 ToMat4(const aiMatrix4x4& m) {
//    return glm::mat4(m.a1, m.b1, m.c1, m.d1, m.a2, m.b2, m.c2, m.d2, m.a3, m.b3, m.c3, m.d3, m.a4, m.b4, m.c4, m.d4);
//}
inline glm::mat4 ToMat4(const aiMatrix4x4& m)
{
    return glm::transpose(glm::make_mat4(&m.a1));
}

// Save compressed texture to file
void saveCompressedTexture(const aiTexture* tex, const std::string& filename) {
    size_t size = tex->mWidth; // length in bytes
    std::ofstream out(filename, std::ios::binary);
    out.write(reinterpret_cast<const char*>(tex->pcData), size);
    out.close();
}

// Save raw BGRA texture to file (simple TGA writer for demo)
void saveRawTexture(const aiTexture* tex, const std::string& filename) {
    std::ofstream out(filename, std::ios::binary);

    // Write minimal TGA header
    uint8_t header[18] = {};
    header[2] = 2; // uncompressed true-color
    header[12] = tex->mWidth & 0xFF;
    header[13] = (tex->mWidth >> 8) & 0xFF;
    header[14] = tex->mHeight & 0xFF;
    header[15] = (tex->mHeight >> 8) & 0xFF;
    header[16] = 32; // bits per pixel
    header[17] = 0x20; // origin top-left
    out.write(reinterpret_cast<char*>(header), 18);

    // Write pixel data (BGRA)
    out.write(reinterpret_cast<const char*>(tex->pcData),
        tex->mWidth * tex->mHeight * 4);

    out.close();
}

inline std::pair<std::vector<TextureDescriptor>, std::vector<bool>> ExtractEmbeddedTexture(aiScene const* scene, std::string const& parent = "", std::string const& default_prefix = "") {
    std::vector<TextureDescriptor> embedded_tex;
    std::vector<bool> is_transparent;
    if (!scene->HasTextures()) {
        return { embedded_tex, is_transparent };
    }

    embedded_tex.reserve(scene->mNumTextures);
    for (unsigned int i = 0; i < scene->mNumTextures; i++) {
        const aiTexture* tex = scene->mTextures[i];
        std::string baseName = tex->mFilename.C_Str();
        baseName = baseName.empty() ? default_prefix+std::to_string(i) : baseName;
        TextureDescriptor texdesc;
        std::string filename{parent};
        texdesc.base.m_importer = "texture";
        texdesc.base.m_importer_type = rp::utility::type_hash<TextureDescriptor>::value();

        if (tex->mHeight == 0) {
            // Compressed texture
            std::string ext = tex->achFormatHint; // e.g. "png", "jpg"
            filename = filename + "/" + (texdesc.base.m_name = baseName + "." + ext);
            int x{};
            int y{}; 
            int comp{};
            int status = stbi_info_from_memory(reinterpret_cast<unsigned char*>(tex->pcData), tex->mWidth, &x, &y, &comp);
            texdesc.base.m_source = rp::utility::get_relative_path(filename, rp::utility::working_path());
            is_transparent.emplace_back((status && (comp == 2 || comp == 4)));
            SaveOrOverwriteDescriptors(texdesc, parent);
            saveCompressedTexture(tex, filename);
        }
        else {
            // Raw BGRA texture
            filename = filename + "/" + (texdesc.base.m_name = baseName + ".tga");
            texdesc.base.m_source = rp::utility::get_relative_path(filename, rp::utility::working_path());
            SaveOrOverwriteDescriptors(texdesc, parent);
            saveRawTexture(tex, filename);
            is_transparent.emplace_back(true);
        }
        embedded_tex.emplace_back(texdesc);
    }
    return { embedded_tex, is_transparent };
}

bool IsMaterialTransparent(const aiMaterial* mat, std::vector<bool> const& trans, std::string const& basepath) {
    if (!mat) return false;

    // 1. Check opacity
    float opacity = 1.0f;
    if (mat->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS) {
        if (opacity < 1.0f) {
            return true;
        }
    }

    // 2. Check transparency factor
    float transparencyFactor = 1.0f;
    if (mat->Get(AI_MATKEY_TRANSPARENCYFACTOR, transparencyFactor) == AI_SUCCESS) {
        if (transparencyFactor < 1.0f) {
            return true;
        }
    }

    // 3. Check transparent color
    aiColor4D transparentColor(0, 0, 0, 0);
    if (mat->Get(AI_MATKEY_COLOR_TRANSPARENT, transparentColor) == AI_SUCCESS) {
        if (transparentColor.a < 1.0f) {
            return true;
        }
    }

    int blendFunc; 
    if (mat->Get(AI_MATKEY_BLEND_FUNC, blendFunc) == AI_SUCCESS) { 
        if (blendFunc != aiBlendMode_Default) {
            return true;
        }
    }

    aiColor4D diffuse;
    if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS) {
        if (diffuse.a < 1.0f) {
            return true;
        }
    }


    // 4. Check if diffuse/base color texture has alpha channel
    aiString texPath;
    if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
        if (*texPath.C_Str() == '*') {
            if (int index = std::stoi(texPath.C_Str()+1); index < trans.size()) {
                return trans[index];
            }
            return false;
        }
        else {
            int x{};
            int y{};
            int comp{};
            int status = stbi_info((basepath + '\\' + texPath.C_Str()).c_str(), &x, &y, &comp);
            return (status && (comp == 2 || comp == 4));
        }
    }

    return false;
}


// --- helper: extract material into MaterialDescriptor ---
inline MaterialDescriptor ExtractMaterial(aiMaterial* aimat, std::string const& base_path, std::vector<TextureDescriptor> const& embeddedTexturesGuid, std::vector<bool> const& embeddedTextureTrans) {
    MaterialDescriptor matDesc{};
    aiString name;
    if (AI_SUCCESS == aimat->Get(AI_MATKEY_NAME, name)) {
        matDesc.material.material_name = name.C_Str();
        if (auto sz = matDesc.material.material_name.find("pin:"); sz != std::string::npos) {
            matDesc.material.material_name = matDesc.material.material_name.substr(sz + 4);
        }
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
        if (std::string desc_path{texname + ".desc"}; std::filesystem::exists(desc_path)) {
            return rp::serialization::yaml_serializer::deserialize<TextureDescriptor>(desc_path).base.m_guid;
        }
        else {
            return rp::null_guid;
        }
        } };

    const auto updateMaterialTextureProperties{ [getTextureGuid, &embeddedTexturesGuid](aiMaterial* mat, std::unordered_map<std::string, rp::Guid>& texprop, std::vector<aiTextureType> const& textypes, std::string const& textypename, std::string const& basepath) {
        for (auto textype : textypes) {
            unsigned int tex_ct{mat->GetTextureCount(textype)};
            for (unsigned int i = 0; i < tex_ct; i++)
            {
                aiString str;
                if (AI_SUCCESS == mat->GetTexture(textype, i, &str)) {
                    std::string texpropname = textypename;
                    std::string pf = std::string(str.C_Str()+1);
                    int index{};
                    texprop[texpropname] = (*str.C_Str()=='*' && (index = std::stoi(pf)) < embeddedTexturesGuid.size()) ? embeddedTexturesGuid[index].base.m_guid : getTextureGuid(basepath + '\\' + str.C_Str());
                    return;
                }
            }
        }
        } };

    // Textures
    updateMaterialTextureProperties(aimat, matDesc.material.texture_properties, { aiTextureType_DIFFUSE, aiTextureType_BASE_COLOR }, "u_DiffuseMap", base_path);
    updateMaterialTextureProperties(aimat, matDesc.material.texture_properties, { aiTextureType_NORMALS, aiTextureType_HEIGHT }, "u_NormalMap", base_path);
    updateMaterialTextureProperties(aimat, matDesc.material.texture_properties, { aiTextureType_DISPLACEMENT }, "u_HeightMap", base_path);
    updateMaterialTextureProperties(aimat, matDesc.material.texture_properties, { aiTextureType_METALNESS }, "u_MetallicMap", base_path);
    updateMaterialTextureProperties(aimat, matDesc.material.texture_properties, { aiTextureType_DIFFUSE_ROUGHNESS }, "u_RoughnessMap", base_path);
    updateMaterialTextureProperties(aimat, matDesc.material.texture_properties, { aiTextureType_AMBIENT_OCCLUSION }, "u_AOMap", base_path);
    updateMaterialTextureProperties(aimat, matDesc.material.texture_properties, { aiTextureType_EMISSIVE }, "u_EmissiveMap", base_path);
    updateMaterialTextureProperties(aimat, matDesc.material.texture_properties, { aiTextureType_SPECULAR }, "u_SpecularMap", base_path);


    matDesc.material.vert_name = "main_pbr.vert";
    matDesc.material.frag_name = "main_pbr.frag";
    matDesc.material.blend_mode = IsMaterialTransparent(aimat, embeddedTextureTrans, base_path) ? 1 : 0;
	matDesc.material.cull_mode = 1; // default to back culling, can be extended to read from material properties if needed

    // Assign a new Guid for this material
    matDesc.base.m_guid = rp::Guid::generate();
    matDesc.base.m_importer = "material";
    matDesc.base.m_name = name.C_Str() + std::string("_material");
    matDesc.base.m_importer_type = rp::utility::type_hash<MaterialDescriptor>::value();

    return matDesc;
}

inline SkeletonDescriptor ExtractSkeleton(const aiScene* scene, std::unordered_map<std::string, int>& boneMap, std::string const& nameprefix="") {
    SkeletonDescriptor skelDesc;
    SkeletonResourceData& skel{ skelDesc.skel };

    int nextId = 0;
    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];
        for (unsigned int b = 0; b < mesh->mNumBones; ++b) {
            const aiBone* bone = mesh->mBones[b];
            std::string name(bone->mName.C_Str());

            if (boneMap.find(name) == boneMap.end()) {
                SkeletonResourceData::Bone newBone;
                newBone.m_bone_name = name;
                newBone.m_id = nextId++;
                newBone.m_parent_index = -1; // fill later
                glm::mat4 ivmat = ToMat4(bone->mOffsetMatrix);
                glm::mat4 imat = { 1.f };
                newBone.m_inv_bind_c1 = ivmat[0];
                newBone.m_inv_bind_c2 = ivmat[1];
                newBone.m_inv_bind_c3 = ivmat[2];
                newBone.m_inv_bind_c4 = ivmat[3];

                skel.m_bones.push_back(newBone);
                boneMap[name] = newBone.m_id;
            }
        }
    }

    std::function<void(const aiNode*, int)> traverse = [&](const aiNode* node, int parentIndex) {
        std::string nodeName(node->mName.C_Str());

        auto it = boneMap.find(nodeName);
        if (it != boneMap.end()) {
            int boneIndex = it->second;
            skel.m_bones[boneIndex].m_parent_index = parentIndex;
            parentIndex = boneIndex; // children inherit this as parent
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            traverse(node->mChildren[i], parentIndex);
        }
        };

    traverse(scene->mRootNode, -1);

    skelDesc.base.m_guid = rp::Guid::generate();
    skelDesc.base.m_importer = "skeleton";
    skelDesc.skel.m_name = skelDesc.base.m_name = (nameprefix.empty() ? std::string(scene->mName.C_Str()) : nameprefix) + std::string(".skeleton");
    skelDesc.base.m_importer_type = rp::utility::type_hash<SkeletonDescriptor>::value();

    return skelDesc;
}

inline AnimationDescriptor ExtractAnimation(aiAnimation* anim, [[maybe_unused]] aiScene const* scene, std::unordered_map<std::string, int> const& boneMap, std::string const& nameprefix = "") {
    AnimationDescriptor anidesc;
    aiString const& name = anim->mName;

    anidesc.anim.m_channels.reserve(anim->mNumChannels);
    for (unsigned int c = 0; c < anim->mNumChannels; c++) {
        auto chl = anim->mChannels[c];
        AnimationResourceData::Channel chldata{};
        chldata.m_name = chl->mNodeName.C_Str();
        for (std::uint32_t i{}; i < chl->mNumPositionKeys; i++) {
            auto poskey = chl->mPositionKeys[i];
            chldata.m_positions.emplace(float(poskey.mTime), ToVec3(poskey.mValue));
        }
        for (std::uint32_t i{}; i < chl->mNumRotationKeys; i++) {
            auto rotkey = chl->mRotationKeys[i];
            chldata.m_rotations.emplace(float(rotkey.mTime), ToQuat(rotkey.mValue));
        }
        for (std::uint32_t i{}; i < chl->mNumScalingKeys; i++) {
            auto sclkey = chl->mScalingKeys[i];
            chldata.m_scales.emplace(float(sclkey.mTime), ToVec3(sclkey.mValue));
        }
        auto res_it = boneMap.find(chldata.m_name);
        chldata.m_id = (res_it==boneMap.end()) ? -1 : res_it->second;
        anidesc.anim.m_channels.emplace_back(chldata);
    }

    anidesc.anim.m_duration = float(anim->mDuration);
    anidesc.anim.m_ticks_per_sec = float(anim->mTicksPerSecond);
    anidesc.base.m_guid = rp::Guid::generate();
    anidesc.base.m_importer = "animation";
    anidesc.base.m_name = (name.Empty() ? nameprefix : std::string(name.C_Str())) + std::string(".animation");
    while (anidesc.base.m_name.find("|") != std::string::npos) {
        auto pos = anidesc.base.m_name.find("|");
        anidesc.base.m_name[pos] = '-';
    }
    anidesc.anim.m_name = anidesc.base.m_name;

    anidesc.base.m_importer_type = rp::utility::type_hash<AnimationDescriptor>::value();
    return anidesc;
}

// --- helper: process one aiMesh ---
inline MeshResourceData::Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene, glm::mat4 const& transform, bool extract_material, std::unordered_map<std::string, int> const& boneMap, std::vector<TextureDescriptor> const& embeddedTextures, std::vector<bool> const& embeddedTextureTrans, std::vector<MaterialDescriptor>& outMaterials, std::string const& base_path)
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
            v.m_Weights[j] = 0;
        }

        out.vertices.push_back(v);
    }

    // Extract bone weights from Assimp
    if (mesh->HasBones() && !boneMap.empty()) {
        std::vector<std::vector<std::pair<int, float>>> tempWeights(out.vertices.size());

        for (unsigned int b = 0; b < mesh->mNumBones; b++) {
            const aiBone* bone = mesh->mBones[b];
            std::string boneName(bone->mName.C_Str());

            auto it = boneMap.find(boneName);
            if (it == boneMap.end()) continue;

            int boneID = it->second;

            for (unsigned int w = 0; w < bone->mNumWeights; w++) {
                unsigned int vertexId = bone->mWeights[w].mVertexId;
                float weight = bone->mWeights[w].mWeight;

                if (vertexId >= out.vertices.size()) continue;

                tempWeights[vertexId].push_back({ boneID, weight });
            }
        }

        for (size_t v = 0; v < out.vertices.size(); v++) {
            auto& vert = out.vertices[v];
            auto& weights = tempWeights[v];

            // Sort descending by weight
            std::sort(weights.begin(), weights.end(),
                [](auto& a, auto& b) { return a.second > b.second; });

            // Keep top 4
            float total = 0.0f;
            for (int j = 0; j < MAX_BONE_INFLUENCE; j++) {
                if (j < (int)weights.size()) {
                    vert.m_BoneIDs[j] = weights[j].first;
                    vert.m_Weights[j] = weights[j].second;
                    total += weights[j].second;
                }
                else {
                    vert.m_BoneIDs[j] = -1;
                    vert.m_Weights[j] = 0.0f;
                }
            }

            // Normalize
            if (total > 0.0f) {
                for (int j = 0; j < MAX_BONE_INFLUENCE; j++) {
                    vert.m_Weights[j] /= total;
                }
            }
        }
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
            MaterialDescriptor matDesc = ExtractMaterial(aimat, base_path, embeddedTextures, embeddedTextureTrans);
            slot.material_guid = matDesc.base.m_guid;
            slot.material_slot_name = matDesc.base.m_name;
            outMaterials.push_back(std::move(matDesc));
        }
    }

    out.materials.push_back(slot);
    return out;
}

inline void getNodeMap(const aiScene* scene, std::unordered_map<std::string, int>& nodeMap)
{
    if (!scene || !scene->mRootNode) return;

    int id = 0;
    const auto traverseNode = [](const aiNode* node, std::unordered_map<std::string, int>& nodeMap, int& currentId, auto nodefn)
        {
            if (!node) return;

            // Map the node name to the current ID
            nodeMap[node->mName.C_Str()] = currentId++;

            // Recurse into children
            for (unsigned int i = 0; i < node->mNumChildren; ++i) {
                nodefn(node->mChildren[i], nodeMap, currentId, nodefn);
            }
        };
    traverseNode(scene->mRootNode, nodeMap, id, traverseNode);
}

// --- main ImportModel ---
inline std::vector<std::pair<rp::Guid, MeshResourceData>> ImportModel(ModelDescriptor const& desc) {
    Assimp::Importer importer;
    //importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4); // Limit to 4 bones per vertex
    const aiScene* scene = importer.ReadFile(
        rp::utility::resolve_path(desc.base.m_source),
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        /*aiProcess_LimitBoneWeights |*/
        aiProcess_JoinIdenticalVertices
    );

    if (!scene || !scene->HasMeshes()) {
        throw std::runtime_error("Failed to load model: " + desc.base.m_source);
    }

    unsigned int maxWeights = 0;

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        aiMesh* mesh = scene->mMeshes[m];

        // Track weights per vertex
        std::vector<unsigned int> weightCount(mesh->mNumVertices, 0);

        for (unsigned int b = 0; b < mesh->mNumBones; ++b) {
            aiBone* bone = mesh->mBones[b];
            for (unsigned int w = 0; w < bone->mNumWeights; ++w) {
                const aiVertexWeight& vw = bone->mWeights[w];
                weightCount[vw.mVertexId]++;
            }
        }

        // Find max for this mesh
        for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
            if (weightCount[v] > maxWeights) {
                maxWeights = weightCount[v];
            }
        }
    }

    std::cout << "Max bone weights per vertex: " << maxWeights << std::endl;

    glm::mat4 transform = BuildTransform(desc);

    std::string parent_path{ std::filesystem::path(rp::utility::resolve_path(desc.base.m_source)).parent_path().string() };

    std::string parent{ rp::utility::resolve_path(desc.base.m_source) };
    parent = parent.substr(0, parent.rfind("\\") + 1);

    // Collect all extracted materials
    std::vector<MaterialDescriptor> extractedMaterials;
    ModelNodeDescriptor mdl_node_desc;

    [[maybe_unused]] aiNode* nd = scene->mRootNode;
    std::unordered_map<std::string, int> boneMap;
    if (desc.is_skinned) {
        auto skeldesc = ExtractSkeleton(scene, boneMap, desc.base.m_name);
        SaveOrOverwriteDescriptors(skeldesc, parent);
    }
    else {
        getNodeMap(scene, boneMap);
    }

    auto [extractedTextures, extractedTexturesTrans] { desc.extract_textures ? ExtractEmbeddedTexture(scene, parent, desc.base.m_name) : std::pair{std::vector<TextureDescriptor>{}, std::vector<bool>{}} };
    if (desc.extract_textures) {
        /*for (auto& texdesc : extractedTextures) {
            SaveOrOverwriteDescriptors(texdesc, parent);
        }*/ //done in ExtractEmbeddedTexture()
    }
    auto processNodes = [&desc, &parent_path, &extractedMaterials, &extractedTextures, &extractedTexturesTrans, &boneMap, &mdl_node_desc](aiScene const* sceneptr, glm::mat4 const& basetransform) {
        auto processNode = [&](aiNode const* ndptr, aiScene const* sptr, glm::mat4 const& parentTransform, std::vector<std::pair<rp::Guid, MeshResourceData>>& mres, int parentidx, auto nodeFn) {
            if (!ndptr)
                return;
            //dont bake transform hierarchies if skinned
            glm::mat4 nodeWorldTransform = parentTransform;
            glm::mat4 nodeLocalTransform = ToMat4(ndptr->mTransformation);
            int nodeid = static_cast<int>(mdl_node_desc.meta.node_local_bind.size());
            mdl_node_desc.meta.node_local_bind.emplace_back(mtx44{nodeLocalTransform[0], nodeLocalTransform[1], nodeLocalTransform[2], nodeLocalTransform[3]});
            mdl_node_desc.meta.node_parent.emplace_back(parentidx);
            for (unsigned int m = 0; m < ndptr->mNumMeshes; m++) {
                MeshResourceData::Mesh mesh = ProcessMesh(sptr->mMeshes[ndptr->mMeshes[m]], sptr, nodeWorldTransform, desc.extract_material, boneMap, extractedTextures, extractedTexturesTrans, extractedMaterials, parent_path);
                if (desc.merge_mesh) {
                    if (mres.empty()) {
                        MeshResourceData mrdata;
                        mrdata.meshes.emplace_back(mesh);
                        mres.emplace_back(std::pair<rp::Guid, MeshResourceData>(desc.base.m_guid, std::move(mrdata)));
                        // adjust material slot
                        for ([[maybe_unused]] auto& slot : mesh.materials) {
                            mdl_node_desc.meta.mesh_node_idx.emplace_back(nodeid);
                        }
                    }
                    else {
                        MeshResourceData::Mesh& mergedMesh = mres[0].second.meshes[0];
                        unsigned int index_vertex_offset{ static_cast<unsigned int>(mergedMesh.vertices.size()) };
                        unsigned int index_size{ static_cast<unsigned int>(mergedMesh.indices.size()) };
                        
                        // Reorder for better GPU vertex cache usage
                        meshopt_optimizeVertexCache(mesh.indices.data(), mesh.indices.data(), mesh.indices.size(), mesh.vertices.size());

                        // Optimize overdraw (reduces pixel shading work)
                        meshopt_optimizeOverdraw(mesh.indices.data(), mesh.indices.data(), mesh.indices.size(),
                            &mesh.vertices[0].Position.x, mesh.vertices.size(), sizeof(MeshResourceData::Vertex), 1.05f);

                        // Optimize vertex fetch (improves memory access)
                        meshopt_optimizeVertexFetch(mesh.vertices.data(), mesh.indices.data(), mesh.indices.size(),
                            mesh.vertices.data(), mesh.vertices.size(), sizeof(MeshResourceData::Vertex));

                        mergedMesh.vertices.insert(mergedMesh.vertices.end(), mesh.vertices.begin(), mesh.vertices.end());

                        // append indices with offset
                        for (auto idx : mesh.indices) {
                            mergedMesh.indices.push_back(idx + index_vertex_offset);
                        }

                        // adjust material slot
                        for (auto& slot : mesh.materials) {
                            MeshResourceData::MaterialSlot newSlot = slot;
                            newSlot.index_begin = index_size;
                            newSlot.material_guid = slot.material_guid;
                            newSlot.material_slot_name = slot.material_slot_name;
                            mergedMesh.materials.push_back(newSlot);
                            mdl_node_desc.meta.mesh_node_idx.emplace_back(nodeid);
                        }
                    }
                }
                else {
                    // Keep all submeshes in a single MeshResourceData with the descriptor's GUID
                    if (mres.empty()) {
                        MeshResourceData mrdata;
                        mrdata.meshes.emplace_back(mesh);
                        mres.emplace_back(std::pair<rp::Guid, MeshResourceData>(desc.base.m_guid, std::move(mrdata)));
                    }
                    else {
                        mres[0].second.meshes.emplace_back(mesh);
                    }
                    for ([[maybe_unused]] auto& slot : mesh.materials) {
                        mdl_node_desc.meta.mesh_node_idx.emplace_back(nodeid);
                    }
                }
            }
            for (unsigned int i = 0; i < ndptr->mNumChildren; i++) {
                nodeFn(ndptr->mChildren[i], sptr, nodeWorldTransform, mres, nodeid, nodeFn);
            }
            };
        std::vector<std::pair<rp::Guid, MeshResourceData>> res;
        processNode(sceneptr->mRootNode, sceneptr, basetransform, res, -1, processNode);
        return res;
        };

    std::vector<std::pair<rp::Guid, MeshResourceData>> result{ processNodes(scene, transform) };

    for (auto& matDesc : extractedMaterials) {
        SaveOrOverwriteDescriptors(matDesc, parent);
    }
        
    if (!desc.merge_mesh) {
        for (auto& [g, mesh] : result) {
            for (auto& submesh : mesh.meshes) {
                // Reorder for better GPU vertex cache usage
                meshopt_optimizeVertexCache(submesh.indices.data(), submesh.indices.data(), submesh.indices.size(), submesh.vertices.size());

                // Optimize overdraw (reduces pixel shading work)
                meshopt_optimizeOverdraw(submesh.indices.data(), submesh.indices.data(), submesh.indices.size(),
                    &submesh.vertices[0].Position.x, submesh.vertices.size(), sizeof(MeshResourceData::Vertex), 1.05f);

                // Optimize vertex fetch (improves memory access)
                meshopt_optimizeVertexFetch(submesh.vertices.data(), submesh.indices.data(), submesh.indices.size(),
                    submesh.vertices.data(), submesh.vertices.size(), sizeof(MeshResourceData::Vertex));
            }
        }
    }

    if (desc.extract_animation) {
        for (unsigned int a = 0; a < scene->mNumAnimations; a++) {
            auto anim = scene->mAnimations[a];
            auto anidesc = ExtractAnimation(anim, scene, boneMap, desc.base.m_name);
            SaveOrOverwriteDescriptors(anidesc, parent);
        }
    }

    mdl_node_desc.base.m_guid = desc.base.m_guid;
    mdl_node_desc.base.m_guid.m_low += 1;
    mdl_node_desc.base.m_importer = ".meshmeta";
    mdl_node_desc.base.m_name = desc.base.m_name + ".meta";
    mdl_node_desc.base.m_importer_type = rp::utility::type_hash<ModelNodeDescriptor>::value();
    rp::serialization::yaml_serializer::serialize(mdl_node_desc, std::filesystem::path(parent + "/" + mdl_node_desc.base.m_name + ".desc").lexically_normal().string());

    return result;
}

RegisterResourceTypeImporter(ModelDescriptor, MeshResourceData, "mesh", ".mesh", ImportModel, ".gltf", ".glb", ".obj", ".fbx")

#endif