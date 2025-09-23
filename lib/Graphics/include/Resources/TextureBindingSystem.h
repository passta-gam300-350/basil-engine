#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <glad/glad.h>
#include <string>

// Forward declarations
struct Texture;
class Shader;



// Abstract binding interface for future bindless support
class ITextureBindingSystem
{
public:
    virtual ~ITextureBindingSystem() = default;
    
    // Core binding interface
    virtual void BindTextures(const std::vector<Texture>& textures, const std::shared_ptr<Shader>& shader) = 0;
    virtual void UnbindAll() = 0;
    
    // Batch operations for efficiency
    virtual void BeginBatch() = 0;
    virtual void EndBatch() = 0;
};

// Traditional OpenGL binding (current implementation)
class TraditionalTextureBinding : public ITextureBindingSystem
{
public:
    void BindTextures(const std::vector<Texture>& textures, const std::shared_ptr<Shader>& shader) override;
    void UnbindAll() override;
    
    // Batch operations (no-op for traditional binding)
    void BeginBatch() override {}
    void EndBatch() override {}
    
private:
    void SetTextureAvailabilityFlags(const std::vector<Texture>& textures, const std::shared_ptr<Shader>& shader);
    void BindTextureSlots(const std::vector<Texture>& textures, const std::shared_ptr<Shader>& shader);
};

// Bindless texture implementation using SSBO
class BindlessTextureBinding : public ITextureBindingSystem
{
public:
    BindlessTextureBinding();
    ~BindlessTextureBinding();
    
    void BindTextures(const std::vector<Texture>& textures, const std::shared_ptr<Shader>& shader) override;
    void UnbindAll() override;
    
    // Efficient batch operations for bindless
    void BeginBatch() override;
    void EndBatch() override;
    
    // Configuration
    static constexpr uint32_t MAX_BINDLESS_TEXTURES = 1024;
    static constexpr uint32_t TEXTURE_HANDLES_SSBO_BINDING = 1;
    
private:
    // Texture handle data for SSBO (std430 aligned)
    struct TextureHandleData
	{
        GLuint64 handle;        // 8 bytes - bindless texture handle
        uint32_t type;          // 4 bytes - texture type (0=diffuse, 1=normal, etc.)
        uint32_t flags;         // 4 bytes - texture flags
        // Total: 16 bytes per handle (properly aligned)
    };

    std::unordered_map<unsigned int, GLuint64> m_HandleCache;
    std::vector<TextureHandleData> m_HandleData;
    GLuint m_HandlesSSBO;
    bool m_BatchActive;
    bool m_SSBODirty;
    bool m_Initialized;
    
    // Extension function pointers
    bool IsBindlessSupported();
    
    // Handle management
    GLuint64 GetOrCreateHandle(unsigned int textureId);
    void MakeHandleResident(GLuint64 handle);
    void MakeHandleNonResident(GLuint64 handle);
    
    // SSBO management
    void InitializeSSBO();
    void UpdateSSBO();
    void UpdateHandleData(const std::vector<Texture>& textures);
    uint32_t GetTextureTypeIndex(const std::string& type);

    void SetTextureAvailabilityFlags(const std::vector<Texture>& textures, std::shared_ptr<Shader> shader);
};

// Factory for texture binding systems
class TextureBindingFactory {
public:
    enum class BindingType {
        Traditional,
        Bindless
    };
    
    static std::unique_ptr<ITextureBindingSystem> Create(BindingType type);
    static bool IsBindlessSupported();
};