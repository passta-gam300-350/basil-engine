#include <Resources/ResourceManager.h>

ResourceManager* ResourceManager::s_Instance = nullptr;

ResourceManager::ResourceManager()
{
    s_Instance = this;
}

ResourceManager::~ResourceManager()
{
    Shutdown();
    s_Instance = nullptr;
}

void ResourceManager::Initialize()
{
    std::cout << "Initializing ResourceManager..." << std::endl;
    LoadDefaultResources();
    std::cout << "ResourceManager initialized successfully." << std::endl;
}

void ResourceManager::Shutdown()
{
    std::cout << "Shutting down ResourceManager..." << std::endl;

    // Clear all resources
    m_Models.clear();
    m_Shaders.clear();

    std::cout << "ResourceManager shutdown complete." << std::endl;
}

std::shared_ptr<Shader> ResourceManager::LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath)
{
    if (HasShader(name))
    {
        std::cout << "Shader already loaded: " << name << std::endl;
        return GetShader(name);
    }

    try
    {
        auto shader = std::make_shared<Shader>(vertexPath.c_str(), fragmentPath.c_str());
        m_Shaders[name] = shader;
        std::cout << "Shader '" << name << "' loaded successfully" << std::endl;
        return shader;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to load shader '" << name << "': " << e.what() << std::endl;
        return nullptr;
    }
}

std::shared_ptr<Shader> ResourceManager::GetShader(const std::string& name)
{
    auto it = m_Shaders.find(name);
    if (it != m_Shaders.end())
        return it->second;
    
    std::cerr << "Shader not found: " << name << std::endl;
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
        std::cout << "Model already loaded: " << name << std::endl;
        return GetModel(name);
    }

    try
    {
        auto model = std::make_shared<Model>(filepath);
        m_Models[name] = model;
        std::cout << "Model '" << name << "' loaded successfully (" << model->meshes.size() << " meshes)" << std::endl;
        return model;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to load model '" << name << "': " << e.what() << std::endl;
        return nullptr;
    }
}

std::shared_ptr<Model> ResourceManager::GetModel(const std::string& name)
{
    auto it = m_Models.find(name);
    if (it != m_Models.end())
        return it->second;
    
    std::cerr << "Model not found: " << name << std::endl;
    return nullptr;
}

bool ResourceManager::HasModel(const std::string& name) const
{
    return m_Models.find(name) != m_Models.end();
}

void ResourceManager::PrintResourceInfo() const
{
    std::cout << "=== Resource Manager Status ===" << std::endl;
    std::cout << "Shaders: " << m_Shaders.size() << std::endl;
    std::cout << "Models: " << m_Models.size() << std::endl;
    std::cout << "===============================" << std::endl;
}

size_t ResourceManager::GetShaderCount() const
{
    return m_Shaders.size();
}

size_t ResourceManager::GetModelCount() const
{
    return m_Models.size();
}

void ResourceManager::LoadDefaultResources()
{
    // Load any default resources here if needed
    std::cout << "Loading default resources..." << std::endl;
}