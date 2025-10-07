#pragma once

#include <vector>
#include <variant>
#include <memory>
#include <Resources/Shader.h>
#include <Resources/Texture.h>
#include <Resources/TextureSlotManager.h>
#include <Buffer/ShaderStorageBuffer.h>
#include <glm/glm.hpp>
#include <glad/glad.h>

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
        uint32_t mode = GL_TRIANGLES;  // GL_TRIANGLES, GL_LINES, GL_POINTS, etc.
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
        uint32_t mode = GL_TRIANGLES;  // GL_TRIANGLES, GL_LINES, GL_POINTS, etc.
    };

    struct SetShadowUniformsData {
        std::shared_ptr<Shader> shader;
        glm::mat4 lightSpaceMatrix;
        uint32_t shadowMapTexture;
        int shadowMapUnit;
        bool enableShadows;
    };

    struct BlitFramebufferData {
        uint32_t srcFBO;
        uint32_t dstFBO;
        int srcX0, srcY0, srcX1, srcY1;
        int dstX0, dstY0, dstX1, dstY1;
        uint32_t mask;  // GL_COLOR_BUFFER_BIT, etc.
        uint32_t filter; // GL_NEAREST, GL_LINEAR
    };

    struct SetUniformVec3Data {
        std::shared_ptr<Shader> shader;
        std::string uniformName;
        glm::vec3 value;
    };

    struct SetBlendingData {
        bool enable;
        uint32_t srcFactor = GL_SRC_ALPHA;      // Default: GL_SRC_ALPHA
        uint32_t dstFactor = GL_ONE_MINUS_SRC_ALPHA; // Default: GL_ONE_MINUS_SRC_ALPHA
    };

    struct SetLineWidthData {
        float width;
    };

    struct SetDepthTestData
    {
        bool enable;
        uint32_t depthFunc = GL_LESS;  // GL_LESS, GL_LEQUAL, GL_GREATER, etc.
        bool depthWrite = true;        // Enable/disable depth buffer writes
    };

    struct SetFaceCullingData
    {
        bool enable;
        uint32_t cullFace = GL_BACK;  // GL_FRONT, GL_BACK, GL_FRONT_AND_BACK
    };

    struct SetObjectIDData {
        std::shared_ptr<Shader> shader;
        uint32_t objectID;
    };

    struct ReadPixelData {
        int x, y;                      // Screen coordinates to read
        uint32_t* outValue;            // Pointer to store the read value
    };

    struct BindCubemapData
    {
        unsigned int cubemapID;
        uint32_t textureUnit;
        std::shared_ptr<Shader> shader;
        std::string uniformName;
    };

    // Compute shader dispatch
    struct DispatchComputeData
    {
        std::shared_ptr<Shader> computeShader;
        uint32_t groupsX;
        uint32_t groupsY;
        uint32_t groupsZ;
    };

    // Bind texture as image for compute shader read/write
    struct BindImageTextureData
    {
        uint32_t unit;           // Image unit to bind to (0-7 typically)
        uint32_t textureID;      // OpenGL texture handle
        uint32_t level;          // Mipmap level
        bool layered;            // Layered texture (for cubemaps/arrays)
        uint32_t layer;          // Layer to bind (if not layered)
        uint32_t access;         // GL_READ_ONLY, GL_WRITE_ONLY, GL_READ_WRITE
        uint32_t format;         // Internal format (GL_RGBA16F, etc.)
    };

    // Memory barrier for synchronization
    struct MemoryBarrierData
    {
        uint32_t barriers;  // GL_SHADER_STORAGE_BARRIER_BIT, GL_SHADER_IMAGE_ACCESS_BARRIER_BIT, etc.
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
    RenderCommands::BlitFramebufferData,
    RenderCommands::SetUniformVec3Data,
    RenderCommands::SetBlendingData,
    RenderCommands::SetLineWidthData,
    RenderCommands::SetDepthTestData,
    RenderCommands::SetFaceCullingData,
    RenderCommands::SetObjectIDData,
    RenderCommands::ReadPixelData,
    RenderCommands::BindCubemapData,
    RenderCommands::DispatchComputeData,
    RenderCommands::BindImageTextureData,
    RenderCommands::MemoryBarrierData
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
    void ExecuteCommand(const RenderCommands::SetUniformVec3Data& cmd);
    void ExecuteCommand(const RenderCommands::SetBlendingData& cmd);
    void ExecuteCommand(const RenderCommands::SetLineWidthData& cmd);
    void ExecuteCommand(const RenderCommands::SetDepthTestData &cmd);
    void ExecuteCommand(const RenderCommands::SetFaceCullingData &cmd);
    void ExecuteCommand(const RenderCommands::SetObjectIDData& cmd);
    void ExecuteCommand(const RenderCommands::ReadPixelData& cmd);
    void ExecuteCommand(const RenderCommands::BindCubemapData &cmd);
    void ExecuteCommand(const RenderCommands::DispatchComputeData &cmd);
    void ExecuteCommand(const RenderCommands::BindImageTextureData &cmd);
    void ExecuteCommand(const RenderCommands::MemoryBarrierData &cmd);

    // GPU state cleanup
    void CleanupGPUState();
};