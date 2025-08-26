#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <glad/gl.h>

// Forward declarations
struct Texture;
class Shader;

// Abstract binding interface for future bindless support
class ITextureBindingSystem {
public:
    virtual ~ITextureBindingSystem() = default;
    
    // Core binding interface
    virtual void BindTextures(const std::vector<Texture>& textures, std::shared_ptr<Shader> shader) = 0;
    virtual void UnbindAll() = 0;
    
    // Batch operations for efficiency
    virtual void BeginBatch() = 0;
    virtual void EndBatch() = 0;
};

// Traditional OpenGL binding (current implementation)
class TraditionalTextureBinding : public ITextureBindingSystem {
public:
    void BindTextures(const std::vector<Texture>& textures, std::shared_ptr<Shader> shader) override;
    void UnbindAll() override;
    
    // Batch operations (no-op for traditional binding)
    void BeginBatch() override {}
    void EndBatch() override {}
    
private:
    void SetTextureAvailabilityFlags(const std::vector<Texture>& textures, std::shared_ptr<Shader> shader);
    void BindTextureSlots(const std::vector<Texture>& textures, std::shared_ptr<Shader> shader);
};

// Future bindless texture implementation stub
class BindlessTextureBinding : public ITextureBindingSystem {
public:
    BindlessTextureBinding();
    ~BindlessTextureBinding();
    
    void BindTextures(const std::vector<Texture>& textures, std::shared_ptr<Shader> shader) override;
    void UnbindAll() override;
    
    // Efficient batch operations for bindless
    void BeginBatch() override;
    void EndBatch() override;
    
private:
    std::vector<GLuint64> m_TextureHandles;
    std::unordered_map<unsigned int, GLuint64> m_HandleCache;
    bool m_BatchActive;
    
    GLuint64 GetOrCreateHandle(unsigned int textureId);
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