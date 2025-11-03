/******************************************************************************/
/*!
\file   ResourceManager.cpp
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Implementation of resource management for shaders, models, and materials

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include <Resources/ResourceManager.h>
#include <spdlog/spdlog.h>

ResourceManager::ResourceManager()
{
}

ResourceManager::~ResourceManager()
{
    Shutdown();
}

void ResourceManager::Initialize()
{
    spdlog::info("Initializing ResourceManager...");
    LoadDefaultResources();
    spdlog::info("ResourceManager initialized successfully.");
}

void ResourceManager::Shutdown()
{
    if (m_IsShutdown) {
        return;  // Already shut down, avoid double cleanup
    }

    spdlog::info("Shutting down ResourceManager...");

    // Clear all resources in safe order
    m_Materials.clear();  // Clear materials first (they may reference shaders)
    m_Models.clear();     // Clear models (they may reference materials/textures)
    m_Shaders.clear();    // Clear shaders last

    m_IsShutdown = true;
    spdlog::info("ResourceManager shutdown complete.");
}

std::shared_ptr<Shader> ResourceManager::LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath)
{
    if (HasShader(name))
    {
        spdlog::debug("Shader already loaded: {}", name);
        return GetShader(name);
    }

    try
    {
        auto shader = std::make_shared<Shader>(vertexPath.c_str(), fragmentPath.c_str());
        m_Shaders[name] = shader;
        spdlog::info("Shader '{}' loaded successfully", name);
        return shader;
    }
    catch (const std::exception& e)
    {
        spdlog::error("Failed to load shader '{}': {}", name, e.what());
        return nullptr;
    }
}

std::shared_ptr<Shader> ResourceManager::LoadShaderWithGeometry(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath, const std::string& geometryPath)
{
    if (HasShader(name))
    {
        spdlog::debug("Shader with geometry already loaded: {}", name);
        return GetShader(name);
    }

    try
    {
        auto shader = std::make_shared<Shader>(vertexPath.c_str(), fragmentPath.c_str(), geometryPath.c_str());
        m_Shaders[name] = shader;
        spdlog::info("Shader with geometry '{}' loaded successfully", name);
        return shader;
    }
    catch (const std::exception& e)
    {
        spdlog::error("Failed to load shader with geometry '{}': {}", name, e.what());
        return nullptr;
    }
}

std::shared_ptr<Shader> ResourceManager::LoadComputeShader(const std::string& name, const std::string& computePath)
{
    if (HasShader(name))
    {
        spdlog::debug("Compute shader already loaded: {}", name);
        return GetShader(name);
    }

    try
    {
        auto shader = std::make_shared<Shader>(computePath.c_str());
        m_Shaders[name] = shader;
        spdlog::info("Compute shader '{}' loaded successfully", name);
        return shader;
    }
    catch (const std::exception& e)
    {
        spdlog::error("Failed to load compute shader '{}': {}", name, e.what());
        return nullptr;
    }
}

std::shared_ptr<Shader> ResourceManager::GetShader(const std::string& name)
{
    auto it = m_Shaders.find(name);
    if (it != m_Shaders.end()) {
        return it->second;
    }
    
    spdlog::error("Shader not found: {}", name);
    return nullptr;
}

bool ResourceManager::HasShader(const std::string& name) const
{
    return m_Shaders.find(name) != m_Shaders.end();
}

std::shared_ptr<Model> ResourceManager::LoadModel(const std::string& name, const std::string& filepath)
{
    if (HasModel(name))
    {
        spdlog::debug("Model already loaded: {}", name);
        return GetModel(name);
    }

    try
    {
        auto model = std::make_shared<Model>(filepath);
        m_Models[name] = model;
        spdlog::info("Model '{}' loaded successfully ({} meshes)", name, model->meshes.size());
        return model;
    }
    catch (const std::exception& e)
    {
        spdlog::error("Failed to load model '{}': {}", name, e.what());
        return nullptr;
    }
}

std::shared_ptr<Model> ResourceManager::GetModel(const std::string& name)
{
    auto it = m_Models.find(name);
    if (it != m_Models.end()) {
        return it->second;
    }
    
    spdlog::error("Model not found: {}", name);
    return nullptr;
}

bool ResourceManager::HasModel(const std::string& name) const
{
    return m_Models.find(name) != m_Models.end();
}

void ResourceManager::AddMaterial(const std::string& name, const std::shared_ptr<Material>& material)
{
    if (!material) {
        spdlog::error("Cannot add null material: {}", name);
        return;
    }

    m_Materials[name] = material;
    spdlog::debug("Material '{}' added to ResourceManager", name);
}

std::shared_ptr<Material> ResourceManager::GetMaterial(const std::string& name)
{
    auto it = m_Materials.find(name);
    if (it != m_Materials.end()) {
        return it->second;
    }

    spdlog::error("Material not found: {}", name);
    return nullptr;
}

bool ResourceManager::HasMaterial(const std::string& name) const
{
    return m_Materials.find(name) != m_Materials.end();
}

void ResourceManager::PrintResourceInfo() const
{
    spdlog::info("=== Resource Manager Status ===");
    spdlog::info("Shaders: {}", m_Shaders.size());
    spdlog::info("Models: {}", m_Models.size());
    spdlog::info("Materials: {}", m_Materials.size());
    spdlog::info("===============================");
}

size_t ResourceManager::GetShaderCount() const
{
    return m_Shaders.size();
}

size_t ResourceManager::GetModelCount() const
{
    return m_Models.size();
}

size_t ResourceManager::GetMaterialCount() const
{
    return m_Materials.size();
}

void ResourceManager::LoadDefaultResources()
{
    // Load any default resources here if needed
    spdlog::debug("Loading default resources...");
}