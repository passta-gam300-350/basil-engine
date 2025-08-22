#include <Resources/ResourceManager.h>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

ResourceManager *ResourceManager::s_Instance = nullptr;

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
    m_Meshes.clear();
    m_Materials.clear();
    m_Textures.clear();
    m_ShaderLibrary.Clear();

    std::cout << "ResourceManager shutdown complete." << std::endl;
}

std::shared_ptr<Shader> ResourceManager::LoadShader(std::string const &name, std::string const &filepath)
{
    return m_ShaderLibrary.Load(name, filepath);
}

//std::shared_ptr<Shader> ResourceManager::LoadShader(std::string const &vertexSrc, std::string const &fragmentSrc, std::string const &name)
//{
//    auto shader = std::make_shared<Shader>(vertexSrc, fragmentSrc);
//    m_ShaderLibrary.Add(name, shader);
//    return shader;
//}

std::shared_ptr<Shader> ResourceManager::LoadShader(std::string const &name, std::string const &vertexPath, std::string const &fragmentPath)
{
    if (HasShader(name))
    {
        std::cout << "Shader already loaded: " << name << std::endl;
        return GetShader(name);
    }

    // Read vertex shader file
    std::ifstream vertexFile(vertexPath);
    if (!vertexFile.is_open())
    {
        std::cerr << "Failed to open vertex shader file: " << vertexPath << std::endl;
        return nullptr;
    }
    std::string vertexSrc((std::istreambuf_iterator<char>(vertexFile)),
                          std::istreambuf_iterator<char>());
    vertexFile.close();

    // Read fragment shader file
    std::ifstream fragmentFile(fragmentPath);
    if (!fragmentFile.is_open())
    {
        std::cerr << "Failed to open fragment shader file: " << fragmentPath << std::endl;
        return nullptr;
    }
    std::string fragmentSrc((std::istreambuf_iterator<char>(fragmentFile)),
                            std::istreambuf_iterator<char>());
    fragmentFile.close();

    // Create shader from sources
    return m_ShaderLibrary.Load(vertexSrc, fragmentSrc, name);
}

std::shared_ptr<Shader> ResourceManager::GetShader(std::string const &name)
{
    return m_ShaderLibrary.Get(name);
}

bool ResourceManager::HasShader(std::string const &name) const
{
    return m_ShaderLibrary.Exists(name);
}

std::shared_ptr<Texture> ResourceManager::LoadTexture(std::string const &name, std::string const &filepath, Texture::Properties const &props)
{
    if (HasTexture(name))
    {
        std::cout << "Texture already loaded: " << name << std::endl;
        return GetTexture(name);
    }

    if (!fs::exists(filepath))
    {
        std::cerr << "Texture file not found: " << filepath << std::endl;
        return nullptr;
    }

    try
    {
        auto texture = std::make_shared<Texture>(filepath, props);
        if (texture->IsLoaded())
        {
            m_Textures[name] = texture;
            std::cout << "Loaded texture: " << name << " from " << filepath << std::endl;
            return texture;
        }
        else
        {
            std::cerr << "Failed to load texture: " << filepath << std::endl;
            return nullptr;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception loading texture " << name << ": " << e.what() << std::endl;
        return nullptr;
    }
}

std::shared_ptr<Texture> ResourceManager::GetTexture(std::string const &name)
{
    auto it = m_Textures.find(name);
    if (it == m_Textures.end())
    {
        std::cerr << "Texture not found: " << name << std::endl;
        return nullptr;
    }
    return it->second;
}

bool ResourceManager::HasTexture(std::string const &name) const
{
    return m_Textures.find(name) != m_Textures.end();
}

std::shared_ptr<Material> ResourceManager::CreateMaterial(std::string const &name, std::string const &shaderName)
{
    auto shader = GetShader(shaderName);
    if (!shader)
    {
        std::cerr << "Cannot create material " << name << ": shader " << shaderName << " not found!" << std::endl;
        return nullptr;
    }

    return CreateMaterial(name, shader);
}

std::shared_ptr<Material> ResourceManager::CreateMaterial(std::string const &name, std::shared_ptr<Shader> const &shader)
{
    if (HasMaterial(name))
    {
        std::cout << "Material already exists: " << name << std::endl;
        return GetMaterial(name);
    }

    if (!shader)
    {
        std::cerr << "Cannot create material " << name << " with null shader!" << std::endl;
        return nullptr;
    }

    auto material = std::make_shared<Material>(shader, name);
    m_Materials[name] = material;

    std::cout << "Created material: " << name << " using shader: " << shader->GetName() << std::endl;
    return material;
}

std::shared_ptr<Material> ResourceManager::GetMaterial(std::string const &name)
{
    auto it = m_Materials.find(name);
    if (it == m_Materials.end())
    {
        std::cerr << "Material not found: " << name << std::endl;
        return nullptr;
    }
    return it->second;
}

bool ResourceManager::HasMaterial(std::string const &name) const
{
    return m_Materials.find(name) != m_Materials.end();
}

std::shared_ptr<Mesh> ResourceManager::GetMesh(std::string const &name)
{
    auto it = m_Meshes.find(name);
    if (it == m_Meshes.end())
    {
        std::cerr << "Mesh not found: " << name << std::endl;
        return nullptr;
    }
    return it->second;
}

bool ResourceManager::HasMesh(std::string const &name) const
{
    return m_Meshes.find(name) != m_Meshes.end();
}

ModelData ResourceManager::LoadModel(std::string const &name, std::string const &filepath, std::string const &defaultShaderName)
{
    // Check if model already loaded
    if (HasModel(name))
    {
        std::cout << "Model already loaded: " << name << std::endl;
        return GetModel(name);
    }

    // Check if file exists
    if (!fs::exists(filepath))
    {
        std::cerr << "Model file not found: " << filepath << std::endl;
        return {};
    }

    std::cout << "Loading model: " << name << " from " << filepath << std::endl;

    // Load with Assimp
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(filepath,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_SortByPType
    );

    // Check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return {};
    }

    // Extract directory path for texture loading
    std::string directory = filepath.substr(0, filepath.find_last_of('/'));

    // Create model data
    ModelData modelData;
    modelData.name = name;
    modelData.directory = directory;

    // Process the scene recursively (like LearnOpenGL)
    ProcessNode(scene->mRootNode, scene, directory, modelData.meshes, modelData.materials, name, defaultShaderName);

    // Store the model
    m_Models[name] = modelData;

    std::cout << "Successfully loaded model: " << name
        << " (" << modelData.meshes.size() << " meshes, "
        << modelData.materials.size() << " materials)" << std::endl;

    return modelData;
}

ModelData ResourceManager::GetModel(std::string const &name)
{
    auto it = m_Models.find(name);
    if (it == m_Models.end())
    {
        std::cerr << "Model not found: " << name << std::endl;
        return {};
    }
    return it->second;
}

bool ResourceManager::HasModel(std::string const &name) const
{
    return m_Models.find(name) != m_Models.end();
}

void ResourceManager::RemoveShader(std::string const &name)
{
    m_ShaderLibrary.Remove(name);
}

void ResourceManager::RemoveTexture(std::string const &name)
{
    auto it = m_Textures.find(name);
    if (it != m_Textures.end())
    {
        m_Textures.erase(it);
        std::cout << "Removed texture: " << name << std::endl;
    }
}

void ResourceManager::RemoveMaterial(std::string const &name)
{
    auto it = m_Materials.find(name);
    if (it != m_Materials.end())
    {
        m_Materials.erase(it);
        std::cout << "Removed material: " << name << std::endl;
    }
}

void ResourceManager::RemoveMesh(std::string const &name)
{
    auto it = m_Meshes.find(name);
    if (it != m_Meshes.end())
    {
        m_Meshes.erase(it);
        std::cout << "Removed mesh: " << name << std::endl;
    }
}

void ResourceManager::RemoveModel(std::string const &name)
{
    auto it = m_Models.find(name);
    if (it != m_Models.end())
    {
        m_Models.erase(it);
        std::cout << "Removed model: " << name << std::endl;
    }
}

void ResourceManager::PrintResourceInfo() const
{
    std::cout << "\n=== Resource Manager Info ===" << std::endl;
    std::cout << "Shaders: " << GetShaderCount() << std::endl;
    std::cout << "Textures: " << GetTextureCount() << std::endl;
    std::cout << "Materials: " << GetMaterialCount() << std::endl;
    std::cout << "Meshes: " << GetMeshCount() << std::endl;
    std::cout << "Models: " << GetModelCount() << std::endl;
    std::cout << "===========================\n" << std::endl;
}

size_t ResourceManager::GetShaderCount() const
{
    return m_ShaderLibrary.GetShaderNames().size();
}

size_t ResourceManager::GetTextureCount() const
{
    return m_Textures.size();
}

size_t ResourceManager::GetMaterialCount() const
{
    return m_Materials.size();
}

size_t ResourceManager::GetMeshCount() const
{
    return m_Meshes.size();
}

size_t ResourceManager::GetModelCount() const
{
    return m_Models.size();
}

void ResourceManager::ProcessNode(aiNode *node, aiScene const *scene, std::string const &directory,
    std::vector<std::shared_ptr<Mesh>> &meshes,
    std::vector<std::shared_ptr<Material>> &materials,
    std::string const &modelName,
    std::string const &defaultShaderName)
{
    std::cout << "Processing node: " << node->mName.C_Str() << " (" << node->mNumMeshes << " meshes)" << std::endl;

    // Process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // The node object only contains indices to index the actual objects in the scene
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];

        // Generate unique names for mesh and material
        std::string meshName = GenerateUniqueName(modelName, std::string(mesh->mName.C_Str()) + "_mesh_" + std::to_string(i));

        // Process the mesh
        auto processedMesh = ProcessMesh(mesh, scene, directory, meshName);
        if (processedMesh)
        {
            meshes.push_back(processedMesh);

            // Process material for this mesh
            if (mesh->mMaterialIndex >= 0)
            {
                std::string materialName = GenerateUniqueName(modelName, "material_" + std::to_string(mesh->mMaterialIndex));

                // Check if we've already processed this material
                auto existingMaterial = GetMaterial(materialName);
                if (!existingMaterial)
                {
                    aiMaterial *assimpMaterial = scene->mMaterials[mesh->mMaterialIndex];
                    existingMaterial = ProcessMaterial(assimpMaterial, scene, directory, materialName, defaultShaderName);
                }

                if (existingMaterial)
                {
                    processedMesh->SetMaterial(existingMaterial);

                    // Add to materials list if not already there
                    auto it = std::find(materials.begin(), materials.end(), existingMaterial);
                    if (it == materials.end())
                    {
                        materials.push_back(existingMaterial);
                    }
                }
            }
        }
    }

    // After we've processed all meshes, recursively process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene, directory, meshes, materials, modelName, defaultShaderName);
    }

}

std::shared_ptr<Mesh> ResourceManager::ProcessMesh(aiMesh *mesh, aiScene const *scene, std::string const &directory,
    std::string const &meshName)
{
    // Use our Mesh::CreateFromAssimp method
    auto processedMesh = Mesh::CreateFromAssimp(mesh, scene);

    if (processedMesh)
    {
        // Store the mesh in our resource manager
        m_Meshes[meshName] = processedMesh;
        std::cout << "  Processed mesh: " << meshName << std::endl;
    }

    return processedMesh;
}

std::shared_ptr<Material> ResourceManager::ProcessMaterial(aiMaterial *assimpMaterial, aiScene const *scene,
    std::string const &directory, std::string const &materialName,
    std::string const &defaultShaderName)
{
    // Determine which shader to use
    std::shared_ptr<Shader> shader;
    if (!defaultShaderName.empty())
    {
        shader = GetShader(defaultShaderName);
    }

    if (!shader)
    {
        // Try to find a default PBR shader
        shader = GetShader("pbr");
        if (!shader)
        {
            shader = GetShader("default");
        }

        if (!shader)
        {
            std::cerr << "No suitable shader found for material: " << materialName << std::endl;
            return nullptr;
        }
    }

    // Create material
    auto material = std::make_shared<Material>(shader, materialName);

    // Load material properties and textures from Assimp
    material->LoadFromAssimp(assimpMaterial, directory);

    // Store the material
    m_Materials[materialName] = material;

    std::cout << "  Processed material: " << materialName << std::endl;

    return material;
}

void ResourceManager::LoadDefaultResources()
{
    // Load default/fallback resources here
    std::cout << "Loading default resources..." << std::endl;

    // Example: Load default shaders, textures, etc.
    // LoadShader("default", "assets/shaders/default.glsl");
    // LoadTexture("white", "assets/textures/white.png");

    std::cout << "Default resources loaded." << std::endl;
}

std::string ResourceManager::GenerateUniqueName(std::string const &baseName, std::string const &suffix) const
{
    return baseName + "_" + suffix;
}