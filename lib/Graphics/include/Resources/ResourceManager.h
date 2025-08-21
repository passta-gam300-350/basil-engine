#pragma once

#include <Resources/Shader.h>
#include <Resources/Texture.h>
#include <Resources/Material.h>
#include <Resources/Mesh.h>
#include <Resources/ShaderLibrary.h>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <filesystem>

struct ModelData
{
    std::vector<std::shared_ptr<Mesh>> meshes;
    std::vector<std::shared_ptr<Material>> materials;
    std::string directory;
    std::string name;
};

class ResourceManager
{
public:
    ResourceManager();
    ~ResourceManager();

    // Initialization
    void Initialize();
    void Shutdown();

    // Shader management
    std::shared_ptr<Shader> LoadShader(std::string const &name, std::string const &filepath);
    std::shared_ptr<Shader> LoadShader(std::string const &vertexSrc, std::string const &fragmentSrc, std::string const &name);
    std::shared_ptr<Shader> GetShader(std::string const &name);
    bool HasShader(std::string const &name) const;

    // Texture management
    std::shared_ptr<Texture> LoadTexture(std::string const &name, std::string const &filepath, Texture::Properties const &props = {});
    std::shared_ptr<Texture> GetTexture(std::string const &name);
    bool HasTexture(std::string const &name) const;

    // Material management
    std::shared_ptr<Material> CreateMaterial(std::string const &name, std::string const &shaderName);
    std::shared_ptr<Material> CreateMaterial(std::string const &name, std::shared_ptr<Shader> const &shader);
    std::shared_ptr<Material> GetMaterial(std::string const &name);
    bool HasMaterial(std::string const &name) const;

    // Mesh management
    std::shared_ptr<Mesh> GetMesh(std::string const &name);
    bool HasMesh(std::string const &name) const;

    // Model loading (this replaces the Model class functionality)
    ModelData LoadModel(std::string const &name, std::string const &filepath, std::string const &defaultShaderName = "");
    ModelData GetModel(std::string const &name);
    bool HasModel(std::string const &name) const;

    // Utility functions
    void RemoveShader(std::string const &name);
    void RemoveTexture(std::string const &name);
    void RemoveMaterial(std::string const &name);
    void RemoveMesh(std::string const &name);
    void RemoveModel(std::string const &name);

    // Debug/Information
    void PrintResourceInfo() const;
    size_t GetShaderCount() const;
    size_t GetTextureCount() const;
    size_t GetMaterialCount() const;
    size_t GetMeshCount() const;
    size_t GetModelCount() const;

    // Singleton access
    static ResourceManager &Get()
    {
        return *s_Instance;
    }

private:
    // Model loading helpers (recursive Assimp processing)
    void ProcessNode(aiNode *node, aiScene const *scene, std::string const &directory,
        std::vector<std::shared_ptr<Mesh>> &meshes,
        std::vector<std::shared_ptr<Material>> &materials,
        std::string const &modelName,
        std::string const &defaultShaderName);

    std::shared_ptr<Mesh> ProcessMesh(aiMesh *mesh, aiScene const *scene, std::string const &directory,
        std::string const &meshName);

    std::shared_ptr<Material> ProcessMaterial(aiMaterial *assimpMaterial, aiScene const *scene,
        std::string const &directory, std::string const &materialName,
        std::string const &defaultShaderName);

    void LoadDefaultResources();
    std::string GenerateUniqueName(std::string const &baseName, std::string const &suffix) const;

    // Resource storage
    Shaderlibrary m_ShaderLibrary;
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_Textures;
    std::unordered_map<std::string, std::shared_ptr<Material>> m_Materials;
    std::unordered_map<std::string, std::shared_ptr<Mesh>> m_Meshes;
    std::unordered_map<std::string, ModelData> m_Models;

    static ResourceManager *s_Instance;
};