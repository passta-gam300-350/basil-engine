#pragma once

#include <vector>
#include <variant>
#include <memory>
#include <Resources/Shader.h>
#include <Resources/Texture.h>
#include <Resources/TextureBindingSystem.h>
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

    // Sort keys for command ordering
    struct CommandSortKey {
        uint8_t pass;          // Rendering pass (0-255)
        uint32_t material;     // Material ID for batching
        uint16_t mesh;         // Mesh ID
        uint16_t instance;     // Instance ID
        
        uint64_t GetSortKey() const {
            return ((uint64_t)pass << 56) |
                   ((uint64_t)material << 24) |
                   ((uint64_t)mesh << 8) |
                   ((uint64_t)instance);
        }
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
    RenderCommands::DrawElementsInstancedData
>;

// Sortable command with metadata
struct SortableCommand {
    VariantRenderCommand command;
    RenderCommands::CommandSortKey sortKey;
    
    bool operator<(const SortableCommand& other) const {
        return sortKey.GetSortKey() < other.sortKey.GetSortKey();
    }
};

// Modern command buffer with efficient storage and sorting
class RenderCommandBuffer {
public:
    RenderCommandBuffer();
    ~RenderCommandBuffer() = default;
    
    // Command submission
    void Submit(const VariantRenderCommand& command, const RenderCommands::CommandSortKey& sortKey = {});
    
    // Efficient batch operations
    void Clear();
    void Sort();    // Sort commands for optimal rendering
    void Execute(); // Execute all commands
    
    // Texture binding system configuration
    void SetTextureBindingSystem(std::unique_ptr<ITextureBindingSystem> bindingSystem);
    
    // Statistics
    size_t GetCommandCount() const { return m_Commands.size(); }
    size_t GetMemoryUsage() const;

private:
    std::vector<SortableCommand> m_Commands;
    std::unique_ptr<ITextureBindingSystem> m_TextureBindingSystem;
    
    // Command execution visitors
    void ExecuteCommand(const RenderCommands::ClearData& cmd);
    void ExecuteCommand(const RenderCommands::BindShaderData& cmd);
    void ExecuteCommand(const RenderCommands::SetUniformsData& cmd);
    void ExecuteCommand(const RenderCommands::BindTexturesData& cmd);
    void ExecuteCommand(const RenderCommands::DrawElementsData& cmd);
    void ExecuteCommand(const RenderCommands::BindSSBOData& cmd);
    void ExecuteCommand(const RenderCommands::DrawElementsInstancedData& cmd);
};