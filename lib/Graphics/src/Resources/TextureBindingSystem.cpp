#include "Resources/TextureBindingSystem.h"
#include "Resources/Texture.h"
#include "Resources/Shader.h"
#include <iostream>
#include <string>
#include <cstring>
#include <GLFW/glfw3.h>

// Traditional OpenGL binding implementation
void TraditionalTextureBinding::BindTextures(const std::vector<Texture>& textures, std::shared_ptr<Shader> shader) {
    if (!shader) return;
    
    // Set texture availability flags
    SetTextureAvailabilityFlags(textures, shader);
    
    // Bind texture slots
    BindTextureSlots(textures, shader);
}

void TraditionalTextureBinding::UnbindAll() {
    // Reset to texture unit 0
    glActiveTexture(GL_TEXTURE0);
}

void TraditionalTextureBinding::SetTextureAvailabilityFlags(const std::vector<Texture>& textures, std::shared_ptr<Shader> shader) {
    // Analyze textures and set availability flags
    bool hasDiffuse = false, hasNormal = false, hasMetallic = false, 
         hasRoughness = false, hasAO = false, hasEmissive = false;
    
    for (const auto& texture : textures) {
        if (texture.type == "texture_diffuse") hasDiffuse = true;
        else if (texture.type == "texture_normal") hasNormal = true;
        else if (texture.type == "texture_metallic") hasMetallic = true;
        else if (texture.type == "texture_roughness") hasRoughness = true;
        else if (texture.type == "texture_ao") hasAO = true;
        else if (texture.type == "texture_emissive") hasEmissive = true;
    }
    
    // Set texture availability flags in shader
    shader->setBool("u_HasDiffuseMap", hasDiffuse);
    shader->setBool("u_HasNormalMap", hasNormal);
    shader->setBool("u_HasMetallicMap", hasMetallic);
    shader->setBool("u_HasRoughnessMap", hasRoughness);
    shader->setBool("u_HasAOMap", hasAO);
    shader->setBool("u_HasEmissiveMap", hasEmissive);
}

void TraditionalTextureBinding::BindTextureSlots(const std::vector<Texture>& textures, std::shared_ptr<Shader> shader) {
    // Bind textures to texture units
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    unsigned int normalNr = 1;
    unsigned int heightNr = 1;
    
    for (unsigned int i = 0; i < textures.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        
        std::string number;
        std::string name = textures[i].type;
        if (name == "texture_diffuse")
            number = std::to_string(diffuseNr++);
        else if (name == "texture_specular")
            number = std::to_string(specularNr++);
        else if (name == "texture_normal")
            number = std::to_string(normalNr++);
        else if (name == "texture_height")
            number = std::to_string(heightNr++);

        shader->setInt((name + number).c_str(), i);
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }
}

// Bindless texture implementation
BindlessTextureBinding::BindlessTextureBinding() 
    : m_HandlesSSBO(0)
    , m_BatchActive(false)
    , m_SSBODirty(false)
    , m_Initialized(false)
    , m_GetTextureHandleARB(nullptr)
    , m_MakeTextureHandleResidentARB(nullptr)
    , m_MakeTextureHandleNonResidentARB(nullptr) {
    
    // Delay initialization until first use to ensure OpenGL context is ready
    // This prevents null pointer exceptions during construction
}

BindlessTextureBinding::~BindlessTextureBinding() {
    if (!m_Initialized) {
        return;
    }
    
    // Make all handles non-resident
    if (m_MakeTextureHandleNonResidentARB) {
        auto makeNonResident = reinterpret_cast<void(*)(GLuint64)>(m_MakeTextureHandleNonResidentARB);
        for (const auto& pair : m_HandleCache) {
            makeNonResident(pair.second);
        }
    }
    
    // Clean up SSBO
    if (m_HandlesSSBO != 0) {
        glDeleteBuffers(1, &m_HandlesSSBO);
    }
}

void BindlessTextureBinding::BindTextures(const std::vector<Texture>& textures, std::shared_ptr<Shader> shader) {
    // Lazy initialization - only initialize when first used
    if (!m_Initialized) {
        bool extensionSupported = IsBindlessSupported();
        bool extensionsLoaded = LoadBindlessExtensions();
        
        std::cout << "[BindlessTextureBinding] Extension supported: " << (extensionSupported ? "YES" : "NO") << std::endl;
        std::cout << "[BindlessTextureBinding] Extensions loaded: " << (extensionsLoaded ? "YES" : "NO") << std::endl;
        
        if (extensionSupported && extensionsLoaded) {
            InitializeSSBO();
            m_Initialized = true;
            std::cout << "[BindlessTextureBinding] Initialized successfully" << std::endl;
        } else {
            std::cout << "[BindlessTextureBinding] Bindless textures not supported, falling back to traditional binding" << std::endl;
            
            // Print GPU info for debugging
            const GLubyte* vendor = glGetString(GL_VENDOR);
            const GLubyte* renderer = glGetString(GL_RENDERER);
            const GLubyte* version = glGetString(GL_VERSION);
            std::cout << "[GPU Info] Vendor: " << (vendor ? (const char*)vendor : "Unknown") << std::endl;
            std::cout << "[GPU Info] Renderer: " << (renderer ? (const char*)renderer : "Unknown") << std::endl;
            std::cout << "[GPU Info] Version: " << (version ? (const char*)version : "Unknown") << std::endl;
        }
    }
    
    if (!m_Initialized || !shader) {
        // Fallback to traditional binding
        TraditionalTextureBinding traditional;
        traditional.BindTextures(textures, shader);
        return;
    }
    
    // Update handle data for current textures
    UpdateHandleData(textures);
    
    // Upload handles to SSBO if dirty
    if (m_SSBODirty) {
        UpdateSSBO();
    }
    
    // Bind SSBO to shader binding point
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, TEXTURE_HANDLES_SSBO_BINDING, m_HandlesSSBO);
    
    // Set texture availability flags in shader (same as traditional)
    SetTextureAvailabilityFlags(textures, shader);
    
    // Set texture indices for shader access
    std::vector<int> textureIndices(8, -1); // Support up to 8 texture types
    
    for (size_t i = 0; i < textures.size() && i < MAX_BINDLESS_TEXTURES; ++i) {
        uint32_t typeIndex = GetTextureTypeIndex(textures[i].type);
        if (typeIndex < 8) {
            textureIndices[typeIndex] = static_cast<int>(i);
        }
    }
    
    // Set texture index uniforms
    shader->setInt("u_DiffuseIndex", textureIndices[0]);
    shader->setInt("u_NormalIndex", textureIndices[1]);
    shader->setInt("u_MetallicIndex", textureIndices[2]);
    shader->setInt("u_RoughnessIndex", textureIndices[3]);
    shader->setInt("u_AOIndex", textureIndices[4]);
    shader->setInt("u_EmissiveIndex", textureIndices[5]);
    shader->setInt("u_SpecularIndex", textureIndices[6]);
    shader->setInt("u_HeightIndex", textureIndices[7]);
}

void BindlessTextureBinding::UnbindAll() {
    if (m_Initialized) {
        // Unbind SSBO
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, TEXTURE_HANDLES_SSBO_BINDING, 0);
    }
}

void BindlessTextureBinding::BeginBatch() {
    m_BatchActive = true;
    m_TextureHandles.clear();
    m_HandleData.clear();
}

void BindlessTextureBinding::EndBatch() {
    if (m_BatchActive) {
        m_SSBODirty = true;
        m_BatchActive = false;
    }
}

bool BindlessTextureBinding::LoadBindlessExtensions() {
    // Load OpenGL extension functions
    m_GetTextureHandleARB = glfwGetProcAddress("glGetTextureHandleARB");
    m_MakeTextureHandleResidentARB = glfwGetProcAddress("glMakeTextureHandleResidentARB");
    m_MakeTextureHandleNonResidentARB = glfwGetProcAddress("glMakeTextureHandleNonResidentARB");
    
    return (m_GetTextureHandleARB != nullptr && 
            m_MakeTextureHandleResidentARB != nullptr && 
            m_MakeTextureHandleNonResidentARB != nullptr);
}

bool BindlessTextureBinding::IsBindlessSupported() {
    // Check if we have a valid OpenGL context
    if (!glGetIntegerv || !glGetString || !glGetStringi) {
        std::cout << "[BindlessTextureBinding] OpenGL context not ready - deferring check" << std::endl;
        return false;
    }
    
    // Additional context validation
    GLenum error = glGetError(); // Clear any pending errors
    const GLubyte* versionStr = glGetString(GL_VERSION);
    if (!versionStr) {
        std::cout << "[BindlessTextureBinding] OpenGL context not fully initialized" << std::endl;
        return false;
    }
    
    // Get OpenGL version info
    GLint majorVersion = 0, minorVersion = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
    
    if (majorVersion == 0) {
        std::cout << "[BindlessTextureBinding] Failed to get OpenGL version - context may not be current" << std::endl;
        return false;
    }
    
    std::cout << "[BindlessTextureBinding] OpenGL Version: " << majorVersion << "." << minorVersion << std::endl;
    
    // Bindless textures require OpenGL 4.4+ or the extension
    if (majorVersion > 4 || (majorVersion == 4 && minorVersion >= 4)) {
        std::cout << "[BindlessTextureBinding] OpenGL 4.4+ detected, checking for core bindless support" << std::endl;
    }
    
    // Check for ARB_bindless_texture extension
    GLint numExtensions = 0;
    error = glGetError(); // Clear any existing errors
    
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    error = glGetError();
    if (error != GL_NO_ERROR || numExtensions <= 0) {
        std::cout << "[BindlessTextureBinding] Failed to get extensions count. Error: " << error << std::endl;
        return false;
    }
    
    std::cout << "[BindlessTextureBinding] Checking " << numExtensions << " extensions..." << std::endl;
    
    for (GLint i = 0; i < numExtensions; ++i) {
        const GLubyte* extension = glGetStringi(GL_EXTENSIONS, i);
        if (!extension) continue;
        
        const char* extName = (const char*)extension;
        if (strcmp(extName, "GL_ARB_bindless_texture") == 0) {
            std::cout << "[BindlessTextureBinding] Found GL_ARB_bindless_texture extension!" << std::endl;
            return true;
        }
        if (strcmp(extName, "GL_NV_bindless_texture") == 0) {
            std::cout << "[BindlessTextureBinding] Found GL_NV_bindless_texture extension!" << std::endl;
            return true;
        }
    }
    
    std::cout << "[BindlessTextureBinding] Bindless texture extensions not found" << std::endl;
    return false;
}

GLuint64 BindlessTextureBinding::GetOrCreateHandle(unsigned int textureId) {
    if (!m_GetTextureHandleARB) {
        return 0;
    }
    
    // Check cache first
    auto it = m_HandleCache.find(textureId);
    if (it != m_HandleCache.end()) {
        return it->second;
    }
    
    // Create new handle
    auto getHandle = reinterpret_cast<GLuint64(*)(GLuint)>(m_GetTextureHandleARB);
    GLuint64 handle = getHandle(textureId);
    
    if (handle != 0) {
        // Make handle resident
        MakeHandleResident(handle);
        
        // Cache the handle
        m_HandleCache[textureId] = handle;
    }
    
    return handle;
}

void BindlessTextureBinding::MakeHandleResident(GLuint64 handle) {
    if (m_MakeTextureHandleResidentARB && handle != 0) {
        auto makeResident = reinterpret_cast<void(*)(GLuint64)>(m_MakeTextureHandleResidentARB);
        makeResident(handle);
    }
}

void BindlessTextureBinding::MakeHandleNonResident(GLuint64 handle) {
    if (m_MakeTextureHandleNonResidentARB && handle != 0) {
        auto makeNonResident = reinterpret_cast<void(*)(GLuint64)>(m_MakeTextureHandleNonResidentARB);
        makeNonResident(handle);
    }
}

void BindlessTextureBinding::InitializeSSBO() {
    // Create SSBO for texture handles
    glGenBuffers(1, &m_HandlesSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_HandlesSSBO);
    
    // Allocate space for three separate arrays (handles, types, flags)
    size_t totalSize = MAX_BINDLESS_TEXTURES * sizeof(GLuint64) +   // handles array
                      MAX_BINDLESS_TEXTURES * sizeof(uint32_t) +    // types array
                      MAX_BINDLESS_TEXTURES * sizeof(uint32_t);     // flags array
    
    glBufferData(GL_SHADER_STORAGE_BUFFER, totalSize, nullptr, GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void BindlessTextureBinding::UpdateSSBO() {
    if (!m_SSBODirty || m_HandleData.empty()) {
        return;
    }
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_HandlesSSBO);
    
    // Create separate arrays for handles, types, and flags (matching shader layout)
    std::vector<GLuint64> handles;
    std::vector<uint32_t> types;
    std::vector<uint32_t> flags;
    
    for (const auto& data : m_HandleData) {
        handles.push_back(data.handle);
        types.push_back(data.type);
        flags.push_back(data.flags);
    }
    
    // Calculate offsets for each array
    size_t handleOffset = 0;
    size_t typeOffset = handles.size() * sizeof(GLuint64);
    size_t flagOffset = typeOffset + types.size() * sizeof(uint32_t);
    
    // Upload each array separately
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, handleOffset, 
                    handles.size() * sizeof(GLuint64), handles.data());
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, typeOffset,
                    types.size() * sizeof(uint32_t), types.data());
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, flagOffset,
                    flags.size() * sizeof(uint32_t), flags.data());
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    
    m_SSBODirty = false;
}

void BindlessTextureBinding::UpdateHandleData(const std::vector<Texture>& textures) {
    m_HandleData.clear();
    m_HandleData.reserve(textures.size());
    
    for (const auto& texture : textures) {
        GLuint64 handle = GetOrCreateHandle(texture.id);
        
        if (handle != 0) {
            TextureHandleData handleData;
            handleData.handle = handle;
            handleData.type = GetTextureTypeIndex(texture.type);
            handleData.flags = 1; // Mark as valid
            
            m_HandleData.push_back(handleData);
        }
    }
    
    m_SSBODirty = true;
}

uint32_t BindlessTextureBinding::GetTextureTypeIndex(const std::string& type) {
    if (type == "texture_diffuse") return 0;
    if (type == "texture_normal") return 1;
    if (type == "texture_metallic") return 2;
    if (type == "texture_roughness") return 3;
    if (type == "texture_ao") return 4;
    if (type == "texture_emissive") return 5;
    if (type == "texture_specular") return 6;
    if (type == "texture_height") return 7;
    return 0; // Default to diffuse
}

void BindlessTextureBinding::SetTextureAvailabilityFlags(const std::vector<Texture>& textures, std::shared_ptr<Shader> shader) {
    // Same logic as traditional binding for compatibility
    bool hasDiffuse = false, hasNormal = false, hasMetallic = false, 
         hasRoughness = false, hasAO = false, hasEmissive = false;
    
    for (const auto& texture : textures) {
        if (texture.type == "texture_diffuse") hasDiffuse = true;
        else if (texture.type == "texture_normal") hasNormal = true;
        else if (texture.type == "texture_metallic") hasMetallic = true;
        else if (texture.type == "texture_roughness") hasRoughness = true;
        else if (texture.type == "texture_ao") hasAO = true;
        else if (texture.type == "texture_emissive") hasEmissive = true;
    }
    
    shader->setBool("u_HasDiffuseMap", hasDiffuse);
    shader->setBool("u_HasNormalMap", hasNormal);
    shader->setBool("u_HasMetallicMap", hasMetallic);
    shader->setBool("u_HasRoughnessMap", hasRoughness);
    shader->setBool("u_HasAOMap", hasAO);
    shader->setBool("u_HasEmissiveMap", hasEmissive);
}

// Factory implementation
std::unique_ptr<ITextureBindingSystem> TextureBindingFactory::Create(BindingType type) {
    switch (type) {
        case BindingType::Traditional:
            return std::make_unique<TraditionalTextureBinding>();
        case BindingType::Bindless:
            // Always create BindlessTextureBinding when requested
            // It will check support lazily when first used (when context is valid)
            return std::make_unique<BindlessTextureBinding>();
        default:
            return std::make_unique<TraditionalTextureBinding>();
    }
}

bool TextureBindingFactory::IsBindlessSupported() {
    // Check if we have a valid OpenGL context by checking if glGetIntegerv is loaded
    if (!glGetIntegerv) {
        return false;
    }
    
    // Check for ARB_bindless_texture extension
    GLint numExtensions = 0;
    GLenum error = glGetError(); // Clear any existing errors
    
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    error = glGetError();
    if (error != GL_NO_ERROR || numExtensions <= 0) {
        return false;
    }
    
    for (GLint i = 0; i < numExtensions; ++i) {
        const GLubyte* extension = glGetStringi(GL_EXTENSIONS, i);
        if (!extension) continue;
        
        if (strcmp((const char*)extension, "GL_ARB_bindless_texture") == 0) {
            return true;
        }
        
        // Also check for NV_bindless_texture (NVIDIA specific)
        if (strcmp((const char*)extension, "GL_NV_bindless_texture") == 0) {
            return true;
        }
    }
    
    return false;
}