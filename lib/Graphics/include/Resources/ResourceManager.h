#pragma once

#include <Resources/Shader.h>
#include <Resources/Model.h>
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
    std::shared_ptr<Shader> GetShader(const std::string& name);
    bool HasShader(const std::string& name) const;

    // Model management - simplified
    std::shared_ptr<Model> LoadModel(const std::string& name, const std::string& filepath);
    std::shared_ptr<Model> GetModel(const std::string& name);
    bool HasModel(const std::string& name) const;

    // Debug/Information
    void PrintResourceInfo() const;
    size_t GetShaderCount() const;
    size_t GetModelCount() const;

    // Singleton access
    static ResourceManager& Get()
    {
        return *s_Instance;
    }

private:
    void LoadDefaultResources();

    // Resource storage - simplified
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
    std::unordered_map<std::string, std::shared_ptr<Model>> m_Models;

    static ResourceManager* s_Instance;
};