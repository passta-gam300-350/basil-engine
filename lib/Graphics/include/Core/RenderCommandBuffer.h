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

    struct SetUniformFloatData {
        std::shared_ptr<Shader> shader;
        std::string uniformName;
        float value;
    };

    struct SetUniformIntData {
        std::shared_ptr<Shader> shader;
        std::string uniformName;
        int value;
    };

    struct SetUniformBoolData {
        std::shared_ptr<Shader> shader;
        std::string uniformName;
        bool value;
    };

    struct SetUniformMat4ArrayData {
        std::shared_ptr<Shader> shader;
        std::string uniformBaseName;  // e.g., "u_ShadowMatrices" (without array index)
        std::vector<glm::mat4> matrices;
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

    struct BindTextureIDData
    {
        uint32_t textureID;
        uint32_t textureUnit;
        std::shared_ptr<Shader> shader;
        std::string uniformName;
    };

    struct DrawArraysData {
        uint32_t vao;
        uint32_t vertexCount;
        uint32_t mode = GL_TRIANGLES;  // GL_TRIANGLES, GL_LINES, GL_POINTS, etc.
        uint32_t first = 0;            // Starting index
    };

    struct DispatchComputeData {
        std::shared_ptr<Shader> computeShader;
        uint32_t numGroupsX;
        uint32_t numGroupsY;
        uint32_t numGroupsZ;
    };

    struct MemoryBarrierData {
        uint32_t barriers;  // GL_SHADER_STORAGE_BARRIER_BIT, GL_TEXTURE_FETCH_BARRIER_BIT, etc.
    };

    // ===== LIGHTING COMMANDS (Option A) =====
    // Forward declarations for light types
    struct PointLightData {
        glm::vec3 position;
        glm::vec3 color;
        float intensity;
        float constant;
        float linear;
        float quadratic;
    };

    struct DirectionalLightData {
        glm::vec3 direction;
        glm::vec3 color;
        float intensity;
    };

    struct SpotLightData {
        glm::vec3 position;
        glm::vec3 direction;
        glm::vec3 color;
        float intensity;
        float cutOff;
        float outerCutOff;
        float constant;
        float linear;
        float quadratic;
    };

    struct SetPointLightsData {
        std::shared_ptr<Shader> shader;
        std::vector<PointLightData> lights;
    };

    struct SetDirectionalLightsData {
        std::shared_ptr<Shader> shader;
        std::vector<DirectionalLightData> lights;
    };

    struct SetSpotLightsData {
        std::shared_ptr<Shader> shader;
        std::vector<SpotLightData> lights;
    };

    struct SetMaterialPBRData {
        std::shared_ptr<Shader> shader;
        glm::vec3 albedoColor;
        float metallicValue;
        float roughnessValue;
    };

    struct SetDirectionalShadowData {
        std::shared_ptr<Shader> shader;
        glm::mat4 lightSpaceMatrix;
        uint32_t shadowMapTexture;
        int shadowMapUnit;
        bool enableShadows;
    };

    struct SetPointShadowsData {
        std::shared_ptr<Shader> shader;
        std::vector<uint32_t> cubemapTextures;
        std::vector<float> farPlanes;
        int startTextureUnit;  // Starting slot (e.g., 9)
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
    RenderCommands::SetUniformFloatData,
    RenderCommands::SetUniformIntData,
    RenderCommands::SetUniformBoolData,
    RenderCommands::SetUniformMat4ArrayData,
    RenderCommands::SetBlendingData,
    RenderCommands::SetLineWidthData,
    RenderCommands::SetDepthTestData,
    RenderCommands::SetFaceCullingData,
    RenderCommands::SetObjectIDData,
    RenderCommands::ReadPixelData,
    RenderCommands::BindCubemapData,
    RenderCommands::BindTextureIDData,
    RenderCommands::DrawArraysData,
    RenderCommands::DispatchComputeData,
    RenderCommands::MemoryBarrierData,
    // Lighting commands (Option A)
    RenderCommands::SetPointLightsData,
    RenderCommands::SetDirectionalLightsData,
    RenderCommands::SetSpotLightsData,
    RenderCommands::SetMaterialPBRData,
    RenderCommands::SetDirectionalShadowData,
    RenderCommands::SetPointShadowsData
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
    void ExecuteCommand(const RenderCommands::SetUniformFloatData& cmd);
    void ExecuteCommand(const RenderCommands::SetUniformIntData& cmd);
    void ExecuteCommand(const RenderCommands::SetUniformBoolData& cmd);
    void ExecuteCommand(const RenderCommands::SetUniformMat4ArrayData& cmd);
    void ExecuteCommand(const RenderCommands::SetBlendingData& cmd);
    void ExecuteCommand(const RenderCommands::SetLineWidthData& cmd);
    void ExecuteCommand(const RenderCommands::SetDepthTestData &cmd);
    void ExecuteCommand(const RenderCommands::SetFaceCullingData &cmd);
    void ExecuteCommand(const RenderCommands::SetObjectIDData& cmd);
    void ExecuteCommand(const RenderCommands::ReadPixelData& cmd);
    void ExecuteCommand(const RenderCommands::BindCubemapData &cmd);
    void ExecuteCommand(const RenderCommands::BindTextureIDData &cmd);
    void ExecuteCommand(const RenderCommands::DrawArraysData &cmd);
    void ExecuteCommand(const RenderCommands::DispatchComputeData &cmd);
    void ExecuteCommand(const RenderCommands::MemoryBarrierData &cmd);
    // Lighting command execution (Option A)
    void ExecuteCommand(const RenderCommands::SetPointLightsData& cmd);
    void ExecuteCommand(const RenderCommands::SetDirectionalLightsData& cmd);
    void ExecuteCommand(const RenderCommands::SetSpotLightsData& cmd);
    void ExecuteCommand(const RenderCommands::SetMaterialPBRData& cmd);
    void ExecuteCommand(const RenderCommands::SetDirectionalShadowData& cmd);
    void ExecuteCommand(const RenderCommands::SetPointShadowsData& cmd);

    // GPU state cleanup
    void CleanupGPUState();
};