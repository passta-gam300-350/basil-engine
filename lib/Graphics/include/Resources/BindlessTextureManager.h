#pragma once

#include "Resources/TextureHandleCache.h"
#include "Resources/TextureSSBO.h"
#include "Resources/Texture.h"
#include "Resources/Shader.h"
#include <vector>
#include <memory>
#include <string>

class BindlessTextureManager {
public:
    BindlessTextureManager();
    ~BindlessTextureManager() = default;

    // Main interface - bind textures using bindless system
    void BindTextures(const std::vector<Texture>& textures, const std::shared_ptr<Shader>& shader);

    // Batch operations for efficiency
    void BeginBatch();
    void EndBatch();

    // Check if bindless is available
    static bool IsSupported();

    // Compatibility method for interface consistency
    void UnbindAll() {} // No-op for bindless system

private:
    TextureHandleCache m_HandleCache;
    TextureSSBO m_SSBO;
    bool m_BatchActive = false;
    std::vector<TextureHandleData> m_PendingHandles;

    // Helper methods
    void ProcessTextures(const std::vector<Texture>& textures);
    void SetShaderUniforms(const std::vector<Texture>& textures, const std::shared_ptr<Shader>& shader);
    void SetTextureAvailabilityFlags(const std::vector<Texture>& textures, const std::shared_ptr<Shader>& shader);
    void SetTextureIndices(const std::vector<Texture>& textures, const std::shared_ptr<Shader>& shader);

    uint32_t GetTextureTypeIndex(const std::string& type) const;
};