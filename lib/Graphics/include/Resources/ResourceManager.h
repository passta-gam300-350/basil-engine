#pragma once

#include <Resources/Shader.h>
#include <Resources/Model.h>
#include <Resources/Material.h>
#include <unordered_map>
#include <string>
#include <memory>
#include <iostream>

class ResourceManager
{
public:
    ResourceManager();
    ~ResourceManager();

    // Initialization
    void Initialize();
    void Shutdown();

    // Shader management - simplified
    std::shared_ptr<Shader> LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);
    // NEW: Overload for geometry shader support
    std::shared_ptr<Shader> LoadShader(const std::string &name,
        const std::string &vertexPath,
        const std::string &geometryPath,
        const std::string &fragmentPath);

    std::shared_ptr<Shader> GetShader(const std::string& name);
    bool HasShader(const std::string& name) const;

    // Model management - simplified
    std::shared_ptr<Model> LoadModel(const std::string& name, const std::string& filepath);
    std::shared_ptr<Model> GetModel(const std::string& name);
    bool HasModel(const std::string& name) const;

    // Material management
    void AddMaterial(const std::string& name, std::shared_ptr<Material> material);
    std::shared_ptr<Material> GetMaterial(const std::string& name);
    bool HasMaterial(const std::string& name) const;

    // Debug/Information
    void PrintResourceInfo() const;
    size_t GetShaderCount() const;
    size_t GetModelCount() const;
    size_t GetMaterialCount() const;

private:
    void LoadDefaultResources();

    // Resource storage - simplified
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
    std::unordered_map<std::string, std::shared_ptr<Model>> m_Models;
    std::unordered_map<std::string, std::shared_ptr<Material>> m_Materials;

    bool m_IsShutdown = false;
};