#pragma once

#include <vector>
#include <variant>
#include <memory>
#include <Resources/Shader.h>
#include <Resources/Texture.h>
#include <Resources/TextureSlotManager.h>
#include <Buffer/ShaderStorageBuffer.h>
#include <glm/glm.hpp>

// Value-based command types - no heap allocation, efficient copying
namespace RenderCommands {

    // Command data structures (POD types)
    struct ClearData {
        float r, g, b, a;
        bool clearColor;
        bool clearDepth;
    };

    struct BindShaderData {
        std::shared_ptr<Shader> shader;
    };

    struct SetUniformsData {
        std::shared_ptr<Shader> shader;
        glm::mat4 modelMatrix;
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;
        glm::vec3 cameraPosition;
    };

    struct BindTexturesData {
        std::vector<Texture> textures;
        std::shared_ptr<Shader> shader;
    };

    struct DrawElementsData {
        uint32_t vao;
        uint32_t indexCount;
    };

    struct BindSSBOData {
        uint32_t ssboHandle;
        uint32_t bindingPoint;
    };

    struct DrawElementsInstancedData {
        uint32_t vao;
        uint32_t indexCount;
        uint32_t instanceCount;
        uint32_t baseInstance;
    };

    struct SetShadowUniformsData {
        std::shared_ptr<Shader> shader;
        glm::mat4 lightSpaceMatrix;
        uint32_t shadowMapTexture;
        int shadowMapUnit;
    };

    struct BlitFramebufferData {
        uint32_t srcFBO;
        uint32_t dstFBO;
        int srcX0, srcY0, srcX1, srcY1;
        int dstX0, dstY0, dstX1, dstY1;
        uint32_t mask;  // GL_COLOR_BUFFER_BIT, etc.
        uint32_t filter; // GL_NEAREST, GL_LINEAR
    };
}

// Command variant - no virtual function calls, cache-friendly
using VariantRenderCommand = std::variant<
    RenderCommands::ClearData,
    RenderCommands::BindShaderData,
    RenderCommands::SetUniformsData,
    RenderCommands::BindTexturesData,
    RenderCommands::DrawElementsData,
    RenderCommands::BindSSBOData,
    RenderCommands::DrawElementsInstancedData,
    RenderCommands::SetShadowUniformsData,
    RenderCommands::BlitFramebufferData
>;

// Modern command buffer with efficient storage and sorting
class RenderCommandBuffer {
public:
    RenderCommandBuffer();
    ~RenderCommandBuffer() = default;

    // Command submission
    void Submit(const VariantRenderCommand& command);

    // Efficient batch operations
    void Clear();
    void Execute(); // Execute all commands

    // Set texture slot manager (should be called by SceneRenderer)
    void SetTextureSlotManager(TextureSlotManager* textureManager) { m_TextureBindingSystem = textureManager; }


    // Statistics
    size_t GetCommandCount() const { return m_Commands.size(); }
    size_t GetMemoryUsage() const;

private:
    std::vector<VariantRenderCommand> m_Commands;
    TextureSlotManager* m_TextureBindingSystem;
    
    // Command execution visitors
    void ExecuteCommand(const RenderCommands::ClearData& cmd);
    void ExecuteCommand(const RenderCommands::BindShaderData& cmd);
    void ExecuteCommand(const RenderCommands::SetUniformsData& cmd);
    void ExecuteCommand(const RenderCommands::BindTexturesData& cmd);
    void ExecuteCommand(const RenderCommands::DrawElementsData& cmd);
    void ExecuteCommand(const RenderCommands::BindSSBOData& cmd);
    void ExecuteCommand(const RenderCommands::DrawElementsInstancedData& cmd);
    void ExecuteCommand(const RenderCommands::SetShadowUniformsData& cmd);
    void ExecuteCommand(const RenderCommands::BlitFramebufferData& cmd);

    // GPU state cleanup
    void CleanupGPUState();
};