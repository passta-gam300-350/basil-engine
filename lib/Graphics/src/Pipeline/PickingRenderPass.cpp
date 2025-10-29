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
        //spdlog::info("PickingRenderPass::Execute - Skipped (enabled: {}, shader: {})", m_Enabled, m_PickingShader != nullptr);
        return; // Skip if disabled or no picking shader
    }

    //spdlog::info("PickingRenderPass::Execute - Starting picking render...");

    // Update framebuffer size to match main framebuffer
    if (context.frameData.mainColorBuffer) {
        const auto& mainSpec = context.frameData.mainColorBuffer->GetSpecification();
        auto pickingSpec = m_Framebuffer->GetSpecification();
        if (pickingSpec.Width != mainSpec.Width ||
            pickingSpec.Height != mainSpec.Height) {
            spdlog::info("PickingRenderPass: Resizing picking buffer from {}x{} to {}x{} (following main buffer)",
                         pickingSpec.Width, pickingSpec.Height, mainSpec.Width, mainSpec.Height);
            m_Framebuffer->Resize(mainSpec.Width, mainSpec.Height);
            SetViewport(Viewport(0, 0, mainSpec.Width, mainSpec.Height));
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

    //spdlog::info("RenderPickingData - Processing {} renderables", context.renderables.size());

    // Bind the picking shader
    RenderCommands::BindShaderData bindShaderCmd{ m_PickingShader };
    Submit(bindShaderCmd);

    // Render each visible object with its unique ID color
    uint32_t renderedCount = 0;
    for (const auto& renderable : context.renderables) {
        if (!renderable.visible || !renderable.mesh || renderable.objectID == 0) {
            spdlog::info("Skipping renderable: visible={}, mesh={}, objectID={}",
                        renderable.visible, (renderable.mesh != nullptr), renderable.objectID);
            continue; // Skip invisible objects or objects without picking IDs
        }

        //spdlog::info("Rendering object with ID: {}", renderable.objectID);
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

    //spdlog::info("RenderPickingData - Rendered {} objects with valid IDs out of {} total renderables", renderedCount, context.renderables.size());
}

PickingResult PickingRenderPass::QueryPicking(const MousePickingQuery& query, const RenderContext& context)
{
    PickingResult result{};

    if (!m_Enabled || !m_Framebuffer) {
        return result;
    }

    // Ensure picking buffer matches main buffer size before querying
    // This is critical because picking may be queried before Execute() runs
    if (context.frameData.mainColorBuffer) {
        const auto& mainSpec = context.frameData.mainColorBuffer->GetSpecification();
        auto pickingSpec = m_Framebuffer->GetSpecification();
        if (pickingSpec.Width != mainSpec.Width || pickingSpec.Height != mainSpec.Height) {
            spdlog::info("QueryPicking: Resizing buffer from {}x{} to {}x{} before query",
                         pickingSpec.Width, pickingSpec.Height, mainSpec.Width, mainSpec.Height);
            m_Framebuffer->Resize(mainSpec.Width, mainSpec.Height);
            SetViewport(Viewport(0, 0, mainSpec.Width, mainSpec.Height));
        }
    }

    // Convert screen coordinates to framebuffer coordinates
    int fbX, fbY;
    ScreenToFramebuffer(query.screenX, query.screenY,
                       query.viewportWidth, query.viewportHeight,
                       fbX, fbY);

    auto fbSpec = m_Framebuffer->GetSpecification();
    spdlog::info("QueryPicking - Screen({}, {}) in Viewport({}x{}) -> Framebuffer({}, {}) [FB Size: {}x{}]",
                query.screenX, query.screenY, query.viewportWidth, query.viewportHeight,
                fbX, fbY, fbSpec.Width, fbSpec.Height);

    // Bind the picking framebuffer for reading
    m_Framebuffer->Bind();

    // Read pixel at the specified location
    uint32_t objectID = 0;
    RenderCommands::ReadPixelData readCmd{ fbX, fbY, &objectID };

    // Execute the read command immediately (not through command buffer)
    // Since this is a query operation that needs immediate result
    uint8_t pixel[4];
    glReadPixels(fbX, fbY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    spdlog::info("QueryPicking - Read pixel RGBA at ({}, {}): ({}, {}, {}, {})",
                 fbX, fbY, pixel[0], pixel[1], pixel[2], pixel[3]);

    // Convert color back to object ID (match shader bit layout)
    objectID = (static_cast<uint32_t>(pixel[0]) << 16) |  // RED → high bits (16-23)
               (static_cast<uint32_t>(pixel[1]) << 8) |   // GREEN → mid bits (8-15)
               static_cast<uint32_t>(pixel[2]);

    spdlog::info("QueryPicking - Converted to object ID: {}", objectID);

    // Read depth value for world position calculation
    float depth;
    glReadPixels(fbX, fbY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

    //spdlog::info("QueryPicking - Depth value: {}", depth);

    m_Framebuffer->Unbind();

    // Fill result
    result.objectID = objectID;
    result.depth = depth;
    result.hasHit = (objectID != 0 && depth < 1.0f);

    if (result.hasHit) {
        // Calculate world position from screen coordinates and depth
        // Convert screen coords to NDC
        float ndcX = (2.0f * fbX) / m_Framebuffer->GetSpecification().Width - 1.0f;
        float ndcY = 1.0f - (2.0f * fbY) / m_Framebuffer->GetSpecification().Height;
        float ndcZ = 2.0f * depth - 1.0f;

        // Convert to clip space
        glm::vec4 clipPos(ndcX, ndcY, ndcZ, 1.0f);

        // Convert to world space
        glm::mat4 invViewProj = glm::inverse(context.frameData.projectionMatrix * context.frameData.viewMatrix);
        glm::vec4 worldPos = invViewProj * clipPos;
        worldPos /= worldPos.w;

        result.worldPosition = glm::vec3(worldPos);
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

glm::vec3 PickingRenderPass::ObjectIDToColor(uint32_t objectID) const
{
    // Convert 24-bit object ID to RGB color
    float r = static_cast<float>((objectID >> 16) & 0xFF) / 255.0f;
    float g = static_cast<float>((objectID >> 8) & 0xFF) / 255.0f;
    float b = static_cast<float>(objectID & 0xFF) / 255.0f;
    return glm::vec3(r, g, b);
}

uint32_t PickingRenderPass::ColorToObjectID(const glm::vec3& color) const
{
    // Convert RGB color back to 24-bit object ID
    uint32_t r = static_cast<uint32_t>(color.r * 255.0f) & 0xFF;
    uint32_t g = static_cast<uint32_t>(color.g * 255.0f) & 0xFF;
    uint32_t b = static_cast<uint32_t>(color.b * 255.0f) & 0xFF;
    return (r << 16) | (g << 8) | b;
}