#include "Resources/TextureBindingSystem.h"
#include "Resources/Texture.h"
#include "Resources/Shader.h"
#include <iostream>
#include <string>

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

// Bindless texture implementation (stub for future)
BindlessTextureBinding::BindlessTextureBinding() : m_BatchActive(false) {
    // Future: Check for ARB_bindless_texture support
}

BindlessTextureBinding::~BindlessTextureBinding() {
    // Future: Clean up bindless handles
}

void BindlessTextureBinding::BindTextures(const std::vector<Texture>& textures, std::shared_ptr<Shader> shader) {
    // Future bindless implementation:
    // 1. Get/create texture handles
    // 2. Upload handle array to GPU
    // 3. Set availability flags
    // 4. No glActiveTexture/glBindTexture calls needed
    
    // For now, fallback to traditional binding
    TraditionalTextureBinding traditional;
    traditional.BindTextures(textures, shader);
}

void BindlessTextureBinding::UnbindAll() {
    // Future: No explicit unbinding needed for bindless
}

void BindlessTextureBinding::BeginBatch() {
    m_BatchActive = true;
    m_TextureHandles.clear();
}

void BindlessTextureBinding::EndBatch() {
    if (m_BatchActive) {
        // Future: Upload batched handles to GPU buffer
        m_BatchActive = false;
    }
}

GLuint64 BindlessTextureBinding::GetOrCreateHandle(unsigned int textureId) {
    // Future: Create/cache bindless texture handles
    auto it = m_HandleCache.find(textureId);
    if (it != m_HandleCache.end()) {
        return it->second;
    }
    
    // Future: GLuint64 handle = glGetTextureHandleARB(textureId);
    // Future: glMakeTextureHandleResidentARB(handle);
    // Future: m_HandleCache[textureId] = handle;
    // Future: return handle;
    
    return 0; // Placeholder
}

// Factory implementation
std::unique_ptr<ITextureBindingSystem> TextureBindingFactory::Create(BindingType type) {
    switch (type) {
        case BindingType::Traditional:
            return std::make_unique<TraditionalTextureBinding>();
        case BindingType::Bindless:
            if (IsBindlessSupported()) {
                return std::make_unique<BindlessTextureBinding>();
            } else {
                std::cout << "Bindless textures not supported, falling back to traditional binding" << std::endl;
                return std::make_unique<TraditionalTextureBinding>();
            }
        default:
            return std::make_unique<TraditionalTextureBinding>();
    }
}

bool TextureBindingFactory::IsBindlessSupported() {
    // Future: Check for ARB_bindless_texture extension
    // const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
    // return strstr(extensions, "GL_ARB_bindless_texture") != nullptr;
    return false; // Not implemented yet
}