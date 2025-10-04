#include "../../include/Pipeline/PickingRenderPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Resources/Shader.h"
#include "../../include/Rendering/InstancedRenderer.h"
#include <glm/ext.hpp>
#include <glad/glad.h>
#include <spdlog/spdlog.h>
#include <cassert>

PickingRenderPass::PickingRenderPass()
    : RenderPass("PickingPass", FBOSpecs{
        1280, 720,
        {
            { FBOTextureFormat::RGBA8 },        // Color buffer for object IDs
            { FBOTextureFormat::DEPTH24STENCIL8 } // Depth buffer for proper depth testing
        }
    }),
      m_PickingShader(nullptr)
{
}

void PickingRenderPass::Execute(RenderContext& context)
{
    if (!m_Enabled || !m_PickingShader) {
        spdlog::info("PickingRenderPass::Execute - Skipped (enabled: {}, shader: {})", m_Enabled, m_PickingShader != nullptr);
        return; // Skip if disabled or no picking shader
    }

    // Assert critical picking system requirements
    assert(m_Framebuffer && "PickingRenderPass must have a valid framebuffer");
    assert(m_PickingShader->ID != 0 && "Picking shader must be compiled and linked");
    assert(!context.renderables.empty() && "No renderables submitted for picking - check entity submission");

    spdlog::info("PickingRenderPass::Execute - Starting picking render...");

    // Update framebuffer size to match main framebuffer
    if (context.frameData.mainColorBuffer) {
        const auto& mainSpec = context.frameData.mainColorBuffer->GetSpecification();
        if (m_Framebuffer->GetSpecification().Width != mainSpec.Width ||
            m_Framebuffer->GetSpecification().Height != mainSpec.Height) {
            m_Framebuffer->Resize(mainSpec.Width, mainSpec.Height);
            SetViewport(Viewport(0, 0, mainSpec.Width, mainSpec.Height));

            // Assert framebuffer is complete after resize
            m_Framebuffer->Bind();
            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            assert(status == GL_FRAMEBUFFER_COMPLETE && "Picking framebuffer must be complete after resize");
            m_Framebuffer->Unbind();
        }
    }

    // Begin the pass
    Begin();

    // Setup command buffer with systems from context
    SetupCommandBuffer(context);

    // Clear to black (object ID 0 = no object)
    RenderCommands::ClearData clearCmd{
        0.0f, 0.0f, 0.0f, 1.0f,  // r, g, b, a (black = no object)
        true,                      // clearColor
        true                       // clearDepth
    };
    Submit(clearCmd);

    // Ensure proper depth testing for picking accuracy
    RenderCommands::SetDepthTestData depthTestCmd{
        true,           // enable depth testing
        GL_LESS,        // depth function
        true            // enable depth writing
    };
    Submit(depthTestCmd);

    // Render picking data (objects with unique ID colors)
    RenderPickingData(context);

    // Execute all commands submitted to this pass's command buffer
    ExecuteCommands();

    // End the pass
    End();
}

void PickingRenderPass::RenderPickingData(RenderContext& context)
{
    if (context.renderables.empty() || !m_PickingShader) {
        spdlog::info("RenderPickingData - Skipped (renderables: {}, shader: {})", context.renderables.size(), m_PickingShader != nullptr);
        return;
    }

    spdlog::info("RenderPickingData - Processing {} renderables", context.renderables.size());

    // Assert shader is bound before rendering
    assert(m_PickingShader && "Picking shader must be valid");
    assert(m_PickingShader->ID != 0 && "Picking shader must have valid OpenGL program");

    // Bind the picking shader
    RenderCommands::BindShaderData bindShaderCmd{ m_PickingShader };
    Submit(bindShaderCmd);

    // Render each visible object with its unique ID color
    uint32_t renderedCount = 0;
    uint32_t validObjectIDCount = 0;

    for (const auto& renderable : context.renderables) {
        // Count objects with valid picking IDs for debugging
        if (renderable.objectID != 0) {
            validObjectIDCount++;
        }

        if (!renderable.visible || !renderable.mesh || renderable.objectID == 0) {
            spdlog::info("Skipping renderable: visible={}, mesh={}, objectID={}",
                        renderable.visible, (renderable.mesh != nullptr), renderable.objectID);
            continue; // Skip invisible objects or objects without picking IDs
        }

        // Assert object can be properly rendered for picking
        assert(renderable.objectID > 0 && renderable.objectID < 16777215 && "Object ID must be in valid 24-bit range");
        assert(renderable.mesh->GetVertexArray() && "Renderable mesh must have valid VAO");
        assert(renderable.mesh->GetIndexCount() > 0 && "Renderable mesh must have indices for drawing");

        spdlog::info("Rendering object with ID: {}", renderable.objectID);
        renderedCount++;

        // Set camera uniforms
        RenderCommands::SetUniformsData uniformsCmd{
            m_PickingShader,
            renderable.transform,
            context.frameData.viewMatrix,
            context.frameData.projectionMatrix,
            context.frameData.cameraPosition
        };
        Submit(uniformsCmd);

        // Set object ID as uniform (shader converts to color)
        RenderCommands::SetObjectIDData objectIDCmd{
            m_PickingShader,
            renderable.objectID
        };
        Submit(objectIDCmd);

        // Draw the mesh
        auto vao = renderable.mesh->GetVertexArray();
        if (vao && vao->GetVAOHandle() != 0) {
            RenderCommands::DrawElementsData drawCmd{
                vao->GetVAOHandle(),
                static_cast<uint32_t>(renderable.mesh->GetIndexCount()),
                GL_TRIANGLES
            };
            Submit(drawCmd);
        }
    }

    spdlog::info("RenderPickingData - Rendered {} objects with valid IDs out of {} total renderables ({} had valid object IDs)",
                renderedCount, context.renderables.size(), validObjectIDCount);

    // Assert that we actually rendered some objects for picking to work
    assert(renderedCount > 0 && "No objects were rendered for picking - check entity visibility and object IDs");
}

PickingResult PickingRenderPass::QueryPicking(const MousePickingQuery& query, const RenderContext& context)
{
    PickingResult result{};

    if (!m_Enabled || !m_Framebuffer) {
        return result;
    }

    // Assert picking query parameters are valid
    assert(query.viewportWidth > 0 && query.viewportHeight > 0 && "Viewport dimensions must be positive");
    assert(query.screenX >= 0 && query.screenY >= 0 && "Screen coordinates must be non-negative");
    assert(query.screenX < query.viewportWidth && query.screenY < query.viewportHeight && "Screen coordinates must be within viewport");

    // Convert screen coordinates to framebuffer coordinates
    int fbX = 0;
    int fbY = 0;
    ScreenToFramebuffer(query.screenX, query.screenY,
                       query.viewportWidth, query.viewportHeight,
                       fbX, fbY);

    spdlog::info("QueryPicking - Screen({}, {}) -> Framebuffer({}, {}) [Viewport: {}x{}]",
                query.screenX, query.screenY, fbX, fbY,
                query.viewportWidth, query.viewportHeight);

    // Assert framebuffer coordinates are valid
    const auto& spec = m_Framebuffer->GetSpecification();
    assert(fbX >= 0 && fbX < static_cast<int>(spec.Width) && "Framebuffer X coordinate out of bounds");
    assert(fbY >= 0 && fbY < static_cast<int>(spec.Height) && "Framebuffer Y coordinate out of bounds");

    // Bind the picking framebuffer for reading
    m_Framebuffer->Bind();

    // Assert framebuffer is bound correctly
    GLint currentFB = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &currentFB);
    assert(currentFB == static_cast<GLint>(m_Framebuffer->GetFBOHandle()) && "Picking framebuffer must be bound for reading");

    // Read pixel at the specified location
    uint32_t objectID = 0;
    RenderCommands::ReadPixelData readCmd{ fbX, fbY, &objectID };

    // Execute the read command immediately (not through command buffer)
    // Since this is a query operation that needs immediate result
    uint8_t pixel[4];
    glReadPixels(fbX, fbY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    spdlog::info("QueryPicking - Read pixel RGBA: ({}, {}, {}, {})", pixel[0], pixel[1], pixel[2], pixel[3]);

    // Convert color back to object ID (match shader bit layout)
    objectID = (static_cast<uint32_t>(pixel[0]) << 16) |  // RED → high bits (16-23)
               (static_cast<uint32_t>(pixel[1]) << 8) |   // GREEN → mid bits (8-15)
               static_cast<uint32_t>(pixel[2]);

    spdlog::info("QueryPicking - Converted to object ID: {}", objectID);

    // Debug: If we got all zeros, check if we're reading from the right place
    if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0) {
        spdlog::warn("QueryPicking - Got all zeros! Possible issues:");
        spdlog::warn("  1. No objects rendered at this pixel");
        spdlog::warn("  2. Picking framebuffer not properly rendered to");
        spdlog::warn("  3. Coordinate conversion error");
        spdlog::warn("  4. Shader not outputting correct colors");

        // Check framebuffer status
        GLenum status = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
        spdlog::warn("  Framebuffer status: 0x{:X} (GL_FRAMEBUFFER_COMPLETE = 0x{:X})",
                    status, GL_FRAMEBUFFER_COMPLETE);
    }

    // Read depth value for world position calculation
    float depth = 0.0f;
    glReadPixels(fbX, fbY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

    spdlog::info("QueryPicking - Depth value: {}", depth);

    m_Framebuffer->Unbind();

    // Fill result
    result.objectID = objectID;
    result.depth = depth;
    result.hasHit = (objectID != 0 && depth < 1.0f);

    if (result.hasHit) {
        // Calculate world position from screen coordinates and depth
        // Convert screen coords to NDC
        const auto& fbspec = m_Framebuffer->GetSpecification();
        float ndcX = (2.0f * static_cast<float>(fbX)) / static_cast<float>(fbspec.Width) - 1.0f;
        float ndcY = 1.0f - (2.0f * static_cast<float>(fbY)) / static_cast<float>(fbspec.Height);
        float ndcZ = 2.0f * depth - 1.0f;

        // Convert to clip space
        glm::vec4 clipPos(ndcX, ndcY, ndcZ, 1.0f);

        // Convert to world space
        glm::mat4 invViewProj = glm::inverse(context.frameData.projectionMatrix * context.frameData.viewMatrix);
        glm::vec4 worldPos = invViewProj * clipPos;
        // Extract w component without union warning
        float w = worldPos[3];
        worldPos /= w;

        result.worldPosition = glm::vec3(worldPos[0], worldPos[1], worldPos[2]);
    }

    return result;
}

void PickingRenderPass::ScreenToFramebuffer(int screenX, int screenY, int screenWidth, int screenHeight,
                                           int& fbX, int& fbY) const
{
    const auto& spec = m_Framebuffer->GetSpecification();

    // Convert screen coordinates to framebuffer coordinates
    // Note: OpenGL framebuffer origin is bottom-left, screen origin is typically top-left
    fbX = (screenX * static_cast<int>(spec.Width)) / screenWidth;
    fbY = static_cast<int>(spec.Height) - ((screenY * static_cast<int>(spec.Height)) / screenHeight);

    // Clamp to framebuffer bounds
    fbX = glm::clamp(fbX, 0, static_cast<int>(spec.Width) - 1);
    fbY = glm::clamp(fbY, 0, static_cast<int>(spec.Height) - 1);
}

glm::vec3 PickingRenderPass::ObjectIDToColor(uint32_t objectID)
{
    // Convert 24-bit object ID to RGB color
    float r = static_cast<float>((objectID >> 16U) & 0xFFU) / 255.0f;
    float g = static_cast<float>((objectID >> 8U) & 0xFFU) / 255.0f;
    float b = static_cast<float>(objectID & 0xFFU) / 255.0f;
    return {r, g, b};
}

uint32_t PickingRenderPass::ColorToObjectID(const glm::vec3& color)
{
    // Convert RGB color back to 24-bit object ID
    // Access via array indexing instead of .r, .g, .b to avoid union warnings
    uint32_t r = static_cast<uint32_t>(color[0] * 255.0f) & 0xFFU;
    uint32_t g = static_cast<uint32_t>(color[1] * 255.0f) & 0xFFU;
    uint32_t b = static_cast<uint32_t>(color[2] * 255.0f) & 0xFFU;
    return (r << 16U) | (g << 8U) | b;
}