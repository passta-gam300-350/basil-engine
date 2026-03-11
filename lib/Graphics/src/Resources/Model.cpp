/******************************************************************************/
/*!
\file   Model.cpp
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Implementation of 3D model loading using Assimp with material and texture support

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include <Resources/Model.h>
#include <Resources/Texture.h>
#include <spdlog/spdlog.h>
#include <glad/glad.h>
#include <stb_image.h>

Model::Model(std::string const &path, bool gamma) : gammaCorrection(gamma)
{
    loadModel(path);

}

// Draw() method removed - Model is now pure data
// Rendering is handled by ECS system through individual Mesh objects

void Model::loadModel(std::string const &path)
{
    // read file via ASSIMP
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    // check for errors
    if(scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || scene->mRootNode == nullptr) // if is Not Zero
    {
        spdlog::error("ASSIMP:: {}", importer.GetErrorString());
        return;
    }
    // retrieve the directory path of the filepath
    directory = path.substr(0, path.find_last_of('/'));

    // process ASSIMP's root node recursively
    processNode(scene->mRootNode, scene);

}

void Model::processNode(aiNode *node, const aiScene *scene)
{
    // Get node name for selection grouping
    std::string nodeName = node->mName.C_Str();

    // process each mesh located at the current node
    for(unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        // the node object only contains indices to index the actual objects in the scene.
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
        meshNodeNames.push_back(nodeName);  // Store node name for this mesh
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for(unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene)
{
    // data to fill
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    // walk through each of the mesh's vertices
    for(unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex vertex{};
        glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
        // positions - Assimp's aiVector3D uses union internally
        vector.x = mesh->mVertices[i].x; // NOLINT(cppcoreguidelines-pro-type-union-access)
        vector.y = mesh->mVertices[i].y; // NOLINT(cppcoreguidelines-pro-type-union-access)
        vector.z = mesh->mVertices[i].z; // NOLINT(cppcoreguidelines-pro-type-union-access)
        vertex.Position = vector;
        // normals - Assimp's aiVector3D uses union internally
        if (mesh->HasNormals())
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        }
        // texture coordinates
        if(mesh->mTextureCoords[0] != nullptr) // does the mesh contain texture coordinates?
        {
            glm::vec2 vec;
            // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
            // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
            // Assimp's aiVector3D uses union internally
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
            // tangent - Assimp's aiVector3D uses union internally
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.Tangent = vector;
            // bitangent - Assimp's aiVector3D uses union internally
            vector.x = mesh->mBitangents[i].x;
            vector.y = mesh->mBitangents[i].y;
            vector.z = mesh->mBitangents[i].z;
            vertex.Bitangent = vector;
        }
        else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }
    // now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for(unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        aiFace face = mesh->mFaces[i];
        // retrieve all indices of the face and store them in the indices vector
        for(unsigned int j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }        
    }
    // process materials
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];


    // Load PBR textures using proper Assimp texture types
    // This matches standard MTL keywords: map_Kd, norm, map_Pm, map_Pr, etc.

    // 1. Diffuse/Albedo maps (map_Kd in MTL)
    std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

    // 2. Normal maps - Support both 'norm' and 'map_Bump' keywords
    // 2a. Load from NORMALS type (norm keyword)
    std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal", scene);
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

    // 2b. Load from HEIGHT type (map_Bump/bump keyword - also used for bump/normal mapping)
    std::vector<Texture> bumpMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal", scene);
    textures.insert(textures.end(), bumpMaps.begin(), bumpMaps.end());

    // 3. Metallic maps (map_Pm in MTL - PBR standard)
    std::vector<Texture> metallicMaps = loadMaterialTextures(material, aiTextureType_METALNESS, "texture_metallic", scene);
    textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());

    // 4. Roughness maps (map_Pr in MTL - PBR standard)
    std::vector<Texture> roughnessMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, "texture_roughness", scene);
    textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());

    // 5. Ambient Occlusion maps (map_Ka or dedicated AO keyword)
    std::vector<Texture> aoMaps = loadMaterialTextures(material, aiTextureType_AMBIENT_OCCLUSION, "texture_ao", scene);
    textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());

    // 6. Emissive maps (map_Ke in MTL)
    std::vector<Texture> emissiveMaps = loadMaterialTextures(material, aiTextureType_EMISSIVE, "texture_emissive", scene);
    textures.insert(textures.end(), emissiveMaps.begin(), emissiveMaps.end());

    // 7. Specular maps (map_Ks in MTL - legacy Phong workflow)
    std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", scene);
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    // 8. Displacement maps (disp in MTL - for actual geometric displacement)
    std::vector<Texture> displacementMaps = loadMaterialTextures(material, aiTextureType_DISPLACEMENT, "texture_height", scene);
    textures.insert(textures.end(), displacementMaps.begin(), displacementMaps.end());


    // return a mesh object created from the extracted mesh data
    return Mesh(vertices, indices, textures);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, const std::string& typeName, const aiScene *scene)
{
    std::vector<Texture> textures;
    for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
        bool skip = false;
        for(unsigned int j = 0; j < textures_loaded.size(); j++)
        {
            if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
            {
                textures.push_back(textures_loaded[j]);
                skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                break;
            }
        }
        if(!skip)
        {   // if texture hasn't been loaded already, load it
            Texture texture;
            // Use gamma correction for diffuse/albedo textures (sRGB), but not for normal/roughness/metallic maps (linear)
            bool useGammaCorrection = (typeName == "texture_diffuse");

            // Check if this is an embedded texture (path starts with '*')
            std::string texturePath = str.C_Str();
            if (!texturePath.empty() && texturePath[0] == '*') {
                // Embedded texture (GLB format) - path is "*N" where N is the index
                int textureIndex = std::atoi(texturePath.c_str() + 1);  // Skip '*' and parse index

                if (scene != nullptr && textureIndex >= 0 && textureIndex < static_cast<int>(scene->mNumTextures)) {
                    const aiTexture* embeddedTexture = scene->mTextures[textureIndex];

                    // Create OpenGL texture from embedded data
                    unsigned int textureID;
                    glGenTextures(1, &textureID);

                    if (embeddedTexture->mHeight == 0) {
                        // Compressed format (PNG, JPG, etc. stored in memory)
                        int width, height, nrComponents;
                        unsigned char* data = stbi_load_from_memory(
                            reinterpret_cast<unsigned char*>(embeddedTexture->pcData),
                            embeddedTexture->mWidth,  // mWidth stores the size in bytes for compressed
                            &width, &height, &nrComponents, 0
                        );

                        if (data) {
                            GLenum format = GL_RGB;
                            if (nrComponents == 1) format = GL_RED;
                            else if (nrComponents == 3) format = GL_RGB;
                            else if (nrComponents == 4) format = GL_RGBA;

                            GLenum internalFormat = useGammaCorrection ?
                                (nrComponents == 4 ? GL_SRGB_ALPHA : GL_SRGB) :
                                format;

                            glBindTexture(GL_TEXTURE_2D, textureID);
                            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
                            glGenerateMipmap(GL_TEXTURE_2D);

                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                            stbi_image_free(data);

                            texture.id = textureID;
                            texture.type = typeName;
                            texture.path = texturePath;
                            textures.push_back(texture);
                            textures_loaded.push_back(texture);

                            //spdlog::debug("Loaded embedded texture {} ({}x{}, {} components)",
                            //            texturePath, width, height, nrComponents);
                        } else {
                            spdlog::error("Failed to decode embedded texture: {}", texturePath);
                        }
                    } else {
                        // Uncompressed format (ARGB8888)
                        glBindTexture(GL_TEXTURE_2D, textureID);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, embeddedTexture->mWidth, embeddedTexture->mHeight,
                                   0, GL_BGRA, GL_UNSIGNED_BYTE, embeddedTexture->pcData);
                        glGenerateMipmap(GL_TEXTURE_2D);

                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                        texture.id = textureID;
                        texture.type = typeName;
                        texture.path = texturePath;
                        textures.push_back(texture);
                        textures_loaded.push_back(texture);

                        //spdlog::debug("Loaded embedded uncompressed texture {} ({}x{})",
                        //            texturePath, embeddedTexture->mWidth, embeddedTexture->mHeight);
                    }
                } else {
                    spdlog::error("Invalid embedded texture index: {}", texturePath);
                }
                continue;
            }

            texture.id = TextureLoader::TextureFromFile(str.C_Str(), this->directory, useGammaCorrection);
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.push_back(texture);
            textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
        }
    }
    return textures;
}