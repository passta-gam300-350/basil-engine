/******************************************************************************/
/*!
\file   RenderCommandBuffer.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the RenderCommandBuffer class for efficient batched rendering commands

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <vector>
#include <variant>
#include <memory>
#include <type_traits>
#include <algorithm>
#include <cstring>
#include <Resources/Shader.h>
#include <Resources/Texture.h>
#include <Resources/TextureSlotManager.h>
#include <Buffer/ShaderStorageBuffer.h>
#include <glm/glm.hpp>
#include <glad/glad.h>

// ===== PERFORMANCE OPTIMIZATION FEATURE FLAG =====
// Set to 0 to use new macro-based enum+union+switch implementation (recommended)
// Set to 1 to use old std::variant implementation (for A/B testing)
#ifndef USE_VARIANT_COMMANDS
#define USE_VARIANT_COMMANDS 0  // Default: use new optimized implementation
#endif

// Value-based command types - no heap allocation, efficient copying
namespace RenderCommands {

    // Command data structures (POD types)
    struct ClearData {
        float r, g, b, a;
        bool clearColor;
        bool clearDepth;
        bool clearStencil = false;  // Optional stencil buffer clear
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

    struct SetUniformMat4Data {
        std::shared_ptr<Shader> shader;
        std::string uniformName;
        glm::mat4 value;
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
        uint32_t blendEquation = GL_FUNC_ADD;   // Default: GL_FUNC_ADD
    };

    struct SetViewportData {
        uint32_t x, y, width, height;
    };

    struct SetUniformVec2Data {
        std::shared_ptr<Shader> shader;
        std::string uniformName;
        glm::vec2 value;
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

    struct BindTexture3DData
    {
        uint32_t textureID;
        uint32_t textureUnit;
        std::shared_ptr<Shader> shader;
        std::string uniformName;
    };

    struct BindTexture2DArrayData
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

    struct DrawArraysInstancedData {
        uint32_t vao;
        uint32_t vertexCount;
        uint32_t instanceCount;
        uint32_t mode = GL_TRIANGLES;  // GL_TRIANGLES, GL_LINES, GL_POINTS, etc.
        uint32_t first = 0;            // Starting vertex index
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
        float ambientIntensity;
        float constant;
        float linear;
        float quadratic;
    };

    struct DirectionalLightData {
        glm::vec3 direction;
        glm::vec3 color;
        float intensity;
        float ambientIntensity;
    };

    struct SpotLightData {
        glm::vec3 position;
        glm::vec3 direction;
        glm::vec3 color;
        float intensity;
        float ambientIntensity;
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

    // ===== STENCIL TESTING COMMANDS =====
    struct EnableStencilTestData {
        bool enable;
    };

    struct SetStencilFuncData {
        uint32_t func;       // GL_NEVER, GL_LESS, GL_LEQUAL, GL_GREATER, GL_GEQUAL, GL_EQUAL, GL_NOTEQUAL, GL_ALWAYS
        int ref;             // Reference value (0-255)
        uint32_t mask;       // Mask value (typically 0xFF)
    };

    struct SetStencilOpData {
        uint32_t sfail;      // Action if stencil test fails
        uint32_t dpfail;     // Action if stencil passes but depth fails
        uint32_t dppass;     // Action if both tests pass
                             // Common values: GL_KEEP, GL_ZERO, GL_REPLACE, GL_INCR, GL_INCR_WRAP, GL_DECR, GL_DECR_WRAP, GL_INVERT
    };

    struct SetStencilMaskData {
        uint32_t mask;       // Stencil write mask (0x00 = no write, 0xFF = write all bits)
    };

    struct SetColorMaskData {
        bool r, g, b, a;     // Enable/disable writing to R, G, B, A channels
    };


}

// ===== X-MACRO PATTERN FOR COMMAND BUFFER OPTIMIZATION =====
// This is the single source of truth for all render commands.
// Adding a new command = adding one line here + implementing ExecuteCommand() overload.
// The macro generates enum, union, switch cases, and type mappings automatically.
#define RENDER_COMMAND_LIST \
    X(ClearData) \
    X(BindShaderData) \
    X(SetUniformsData) \
    X(BindTexturesData) \
    X(DrawElementsData) \
    X(BindSSBOData) \
    X(DrawElementsInstancedData) \
    X(BlitFramebufferData) \
    X(SetUniformVec3Data) \
    X(SetUniformFloatData) \
    X(SetUniformIntData) \
    X(SetUniformBoolData) \
    X(SetUniformMat4Data) \
    X(SetUniformMat4ArrayData) \
    X(SetBlendingData) \
    X(SetViewportData) \
    X(SetUniformVec2Data) \
    X(SetLineWidthData) \
    X(SetDepthTestData) \
    X(SetFaceCullingData) \
    X(SetObjectIDData) \
    X(ReadPixelData) \
    X(BindCubemapData) \
    X(BindTextureIDData) \
    X(BindTexture3DData) \
    X(BindTexture2DArrayData) \
    X(DrawArraysData) \
    X(DrawArraysInstancedData) \
    X(DispatchComputeData) \
    X(MemoryBarrierData) \
    X(SetPointLightsData) \
    X(SetDirectionalLightsData) \
    X(SetSpotLightsData) \
    X(SetMaterialPBRData) \
    X(EnableStencilTestData) \
    X(SetStencilFuncData) \
    X(SetStencilOpData) \
    X(SetStencilMaskData) \
    X(SetColorMaskData)

#if !USE_VARIANT_COMMANDS
// ===== AUTO-GENERATED ENUM (39 command types) =====
// DO NOT EDIT - generated by RENDER_COMMAND_LIST macro
enum class RenderCommandType : uint8_t {
    #define X(name) name,
    RENDER_COMMAND_LIST
    #undef X
    COUNT  // Total number of command types
};

// ===== TYPE-SAFE COMMAND WRAPPER =====
// Pointer-based storage to avoid size issues with commands containing vectors
// This matches std::variant's approach but with enum+switch dispatch
struct RenderCommand {
    RenderCommandType type;
    void* data;  // Pointer to heap-allocated command data

    // Default constructor
    RenderCommand() : type(RenderCommandType::ClearData), data(nullptr) {}

    // Template constructor for type-safe construction
    template<typename T>
    explicit RenderCommand(RenderCommandType cmdType, T&& cmd)
        : type(cmdType)
        , data(new std::decay_t<T>(std::forward<T>(cmd)))
    {
    }

    // Destructor - must delete based on type
    ~RenderCommand() {
        if (data) {
            // Switch to call proper destructor based on type
            switch (type) {
                #define X(name) \
                    case RenderCommandType::name: \
                        delete static_cast<RenderCommands::name*>(data); \
                        break;
                RENDER_COMMAND_LIST
                #undef X
                default:
                    break;
            }
        }
    }

    // Copy constructor
    RenderCommand(const RenderCommand& other) : type(other.type), data(nullptr) {
        if (other.data) {
            // Switch to copy construct based on type
            switch (other.type) {
                #define X(name) \
                    case RenderCommandType::name: \
                        data = new RenderCommands::name(*static_cast<const RenderCommands::name*>(other.data)); \
                        break;
                RENDER_COMMAND_LIST
                #undef X
                default:
                    break;
            }
        }
    }

    // Move constructor
    RenderCommand(RenderCommand&& other) noexcept
        : type(other.type), data(other.data)
    {
        other.data = nullptr;
    }

    // Copy assignment
    RenderCommand& operator=(const RenderCommand& other) {
        if (this != &other) {
            this->~RenderCommand();
            new (this) RenderCommand(other);
        }
        return *this;
    }

    // Move assignment
    RenderCommand& operator=(RenderCommand&& other) noexcept {
        if (this != &other) {
            this->~RenderCommand();
            type = other.type;
            data = other.data;
            other.data = nullptr;
        }
        return *this;
    }

    // Get typed pointer to command data
    template<typename T>
    T* as() { return static_cast<T*>(data); }

    template<typename T>
    const T* as() const { return static_cast<const T*>(data); }
};

// ===== TYPE TO ENUM MAPPING =====
// Maps C++ type to enum value at compile time
template<typename T>
constexpr RenderCommandType GetCommandType();

// Generate specializations for each command type
#define X(name) \
    template<> \
    inline constexpr RenderCommandType GetCommandType<RenderCommands::name>() { \
        return RenderCommandType::name; \
    }
RENDER_COMMAND_LIST
#undef X

#endif // !USE_VARIANT_COMMANDS

// ===== DEPRECATED: OLD VARIANT-BASED SYSTEM =====
// Commenting out to identify remaining dependencies
/*
using VariantRenderCommand = std::variant<
    RenderCommands::ClearData,
    RenderCommands::BindShaderData,
    RenderCommands::SetUniformsData,
    RenderCommands::BindTexturesData,
    RenderCommands::DrawElementsData,
    RenderCommands::BindSSBOData,
    RenderCommands::DrawElementsInstancedData,
    RenderCommands::BlitFramebufferData,
    RenderCommands::SetUniformVec3Data,
    RenderCommands::SetUniformFloatData,
    RenderCommands::SetUniformIntData,
    RenderCommands::SetUniformBoolData,
    RenderCommands::SetUniformMat4Data,
    RenderCommands::SetUniformMat4ArrayData,
    RenderCommands::SetBlendingData,
    RenderCommands::SetViewportData,
    RenderCommands::SetUniformVec2Data,
    RenderCommands::SetLineWidthData,
    RenderCommands::SetDepthTestData,
    RenderCommands::SetFaceCullingData,
    RenderCommands::SetObjectIDData,
    RenderCommands::ReadPixelData,
    RenderCommands::BindCubemapData,
    RenderCommands::BindTextureIDData,
    RenderCommands::BindTexture3DData,
    RenderCommands::BindTexture2DArrayData,
    RenderCommands::DrawArraysData,
    RenderCommands::DrawArraysInstancedData,
    RenderCommands::DispatchComputeData,
    RenderCommands::MemoryBarrierData,
    // Lighting commands (Option A)
    RenderCommands::SetPointLightsData,
    RenderCommands::SetDirectionalLightsData,
    RenderCommands::SetSpotLightsData,
    RenderCommands::SetMaterialPBRData,
    // Stencil commands
    RenderCommands::EnableStencilTestData,
    RenderCommands::SetStencilFuncData,
    RenderCommands::SetStencilOpData,
    RenderCommands::SetStencilMaskData,
    // Color mask command
    RenderCommands::SetColorMaskData
>;
*/

// Modern command buffer with efficient storage and sorting
class RenderCommandBuffer {
public:
    RenderCommandBuffer();
    ~RenderCommandBuffer() = default;

    // ===== COMMAND SUBMISSION =====
#if USE_VARIANT_COMMANDS
    // OLD: Variant-based submission (kept for A/B testing)
    // COMMENTED OUT TO FIND DEPENDENCIES
    // void Submit(const VariantRenderCommand& command);
#else
    // NEW: Template overload for macro-based implementation
    // Automatically deduces command type and creates RenderCommand
    // Call sites unchanged: Submit(RenderCommands::ClearData{...});
    template<typename T>
    void Submit(T&& command) {
        m_Commands.emplace_back(RenderCommand{
            GetCommandType<std::decay_t<T>>(),
            std::forward<T>(command)
        });
    }

    /* COMMENTED OUT TO FIND DEPENDENCIES
    // Special overload for VariantRenderCommand (in case old code still uses it)
    // This unwraps the variant and calls the appropriate Submit
    void Submit(const VariantRenderCommand& command) {
        std::visit([this](const auto& cmd) {
            this->Submit(cmd);
        }, command);
    }
    */
#endif

    // Efficient batch operations
    void Clear();
    void Execute(); // Execute all commands

    // Set texture slot manager (should be called by SceneRenderer)
    void SetTextureSlotManager(TextureSlotManager* textureManager) { m_TextureBindingSystem = textureManager; }


    // Statistics
    size_t GetCommandCount() const { return m_Commands.size(); }
    size_t GetMemoryUsage() const;

private:
#if USE_VARIANT_COMMANDS
    // COMMENTED OUT TO FIND DEPENDENCIES
    // std::vector<VariantRenderCommand> m_Commands;  // Old implementation
#else
    std::vector<RenderCommand> m_Commands;  // New optimized implementation
#endif
    TextureSlotManager* m_TextureBindingSystem;

    // Shader state tracking for optimization (avoid redundant binds)
    mutable GLuint m_CurrentShaderID = 0;

    // Command execution visitors
    void ExecuteCommand(const RenderCommands::ClearData& cmd);
    void ExecuteCommand(const RenderCommands::BindShaderData& cmd);
    void ExecuteCommand(const RenderCommands::SetUniformsData& cmd);
    void ExecuteCommand(const RenderCommands::BindTexturesData& cmd);
    void ExecuteCommand(const RenderCommands::DrawElementsData& cmd);
    void ExecuteCommand(const RenderCommands::BindSSBOData& cmd);
    void ExecuteCommand(const RenderCommands::DrawElementsInstancedData& cmd);
    void ExecuteCommand(const RenderCommands::BlitFramebufferData& cmd);
    void ExecuteCommand(const RenderCommands::SetUniformVec3Data& cmd);
    void ExecuteCommand(const RenderCommands::SetUniformFloatData& cmd);
    void ExecuteCommand(const RenderCommands::SetUniformIntData& cmd);
    void ExecuteCommand(const RenderCommands::SetUniformBoolData& cmd);
    void ExecuteCommand(const RenderCommands::SetUniformMat4Data& cmd);
    void ExecuteCommand(const RenderCommands::SetUniformMat4ArrayData& cmd);
    void ExecuteCommand(const RenderCommands::SetUniformVec2Data& cmd);
    void ExecuteCommand(const RenderCommands::SetBlendingData& cmd);
    void ExecuteCommand(const RenderCommands::SetViewportData& cmd);
    void ExecuteCommand(const RenderCommands::SetLineWidthData& cmd);
    void ExecuteCommand(const RenderCommands::SetDepthTestData &cmd);
    void ExecuteCommand(const RenderCommands::SetFaceCullingData &cmd);
    void ExecuteCommand(const RenderCommands::SetObjectIDData& cmd);
    void ExecuteCommand(const RenderCommands::ReadPixelData& cmd);
    void ExecuteCommand(const RenderCommands::BindCubemapData &cmd);
    void ExecuteCommand(const RenderCommands::BindTextureIDData &cmd);
    void ExecuteCommand(const RenderCommands::BindTexture3DData &cmd);
    void ExecuteCommand(const RenderCommands::BindTexture2DArrayData &cmd);
    void ExecuteCommand(const RenderCommands::DrawArraysData &cmd);
    void ExecuteCommand(const RenderCommands::DrawArraysInstancedData &cmd);
    void ExecuteCommand(const RenderCommands::DispatchComputeData &cmd);
    void ExecuteCommand(const RenderCommands::MemoryBarrierData &cmd);
    // Lighting command execution (Option A)
    void ExecuteCommand(const RenderCommands::SetPointLightsData& cmd);
    void ExecuteCommand(const RenderCommands::SetDirectionalLightsData& cmd);
    void ExecuteCommand(const RenderCommands::SetSpotLightsData& cmd);
    void ExecuteCommand(const RenderCommands::SetMaterialPBRData& cmd);
    // Stencil command execution
    void ExecuteCommand(const RenderCommands::EnableStencilTestData& cmd);
    void ExecuteCommand(const RenderCommands::SetStencilFuncData& cmd);
    void ExecuteCommand(const RenderCommands::SetStencilOpData& cmd);
    void ExecuteCommand(const RenderCommands::SetStencilMaskData& cmd);
    // Color mask command execution
    void ExecuteCommand(const RenderCommands::SetColorMaskData& cmd);

    // GPU state cleanup
    void CleanupGPUState();
};