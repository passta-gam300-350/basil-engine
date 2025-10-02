#include "../../include/Pipeline/PointShadowMappingPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Utility/Light.h"
#include "../../include/Resources/Shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

PointShadowMappingPass::PointShadowMappingPass()
    : RenderPass("PointShadowPass"),  // No default FBO - we create cubemaps dynamically
    m_PointShadowDepthShader(nullptr)
{
}

PointShadowMappingPass::PointShadowMappingPass(std::shared_ptr<Shader> depthShader)
    : RenderPass("PointShadowPass"),
    m_PointShadowDepthShader(depthShader)
{
}

void PointShadowMappingPass::Execute(RenderContext &context)
{
    if (!m_PointShadowDepthShader)
    {
        spdlog::warn("PointShadowMappingPass: No depth shader set, skipping");
        return;
    }

    // Clear previous frame's point shadow data
    context.frameData.pointShadows.clear();

    // Find point lights that should cast shadows
    std::vector<const SubmittedLightData *> shadowCastingLights;
    for (const auto &light : context.lights)
    {
        if (light.enabled && light.type == Light::Type::Point && light.castShadows)
        {
            shadowCastingLights.push_back(&light);
            if (shadowCastingLights.size() >= m_MaxShadowCastingLights)
            {
                break;  // Limit number of shadow-casting point lights
            }
        }
    }

    if (shadowCastingLights.empty())
    {
        spdlog::info("PointShadowMappingPass: No shadow-casting lights found");
        return;  // No point lights casting shadows
    }

    spdlog::info("PointShadowMappingPass: Rendering shadows for {} light(s)", shadowCastingLights.size());
    Begin();
    SetupCommandBuffer(context);

    // Render shadows for each point light (SINGLE PASS with geometry shader)
    for (size_t lightIdx = 0; lightIdx < shadowCastingLights.size(); ++lightIdx)
    {
        const auto *light = shadowCastingLights[lightIdx];

        // Get or create cubemap FBO for this light
        auto cubemapFBO = GetOrCreateCubemapFBO(lightIdx);
        spdlog::info("Point shadow light {}: FBO depth cubemap ID = {}", lightIdx, cubemapFBO->GetDepthCubemapID());

        // Calculate view matrices for 6 cubemap faces
        auto viewMatrices = CalculateCubemapViewMatrices(light->position);
        auto projection = CalculatePointShadowProjection();

        // Render to entire cubemap in ONE pass (geometry shader handles all 6 faces)
        RenderToCubemap(context, viewMatrices, projection, light->position, lightIdx);

        // Store shadow data in FrameData
        FrameData::PointShadowData shadowData;
        shadowData.cubemapFBO = cubemapFBO;
        shadowData.viewMatrices = viewMatrices;
        shadowData.projectionMatrix = projection;
        shadowData.lightPosition = light->position;
        shadowData.farPlane = m_ShadowFarPlane;

        context.frameData.pointShadows.push_back(shadowData);
    }

    End();
}

std::array<glm::mat4, 6> PointShadowMappingPass::CalculateCubemapViewMatrices(
    const glm::vec3 &lightPos)
{
    // Standard cubemap view directions and up vectors
    return {
        glm::lookAt(lightPos, lightPos + glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // +X
        glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // -X
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)), // +Y
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)), // -Y
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // +Z
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))  // -Z
    };
}

glm::mat4 PointShadowMappingPass::CalculatePointShadowProjection()
{
    // 90-degree FOV perspective for cubemap faces
    float aspect = 1.0f;  // Cubemap faces are square
    float nearPlane = 0.1f;
    return glm::perspective(glm::radians(90.0f), aspect, nearPlane, m_ShadowFarPlane);
}

std::shared_ptr<FrameBuffer> PointShadowMappingPass::GetOrCreateCubemapFBO(size_t lightIndex)
{
    // Resize vector if needed
    if (lightIndex >= m_CubemapFBOs.size())
    {
        m_CubemapFBOs.resize(lightIndex + 1);
    }

    // Create FBO if it doesn't exist
    if (!m_CubemapFBOs[lightIndex])
    {
        FBOSpecs spec;
        spec.Width = m_ShadowCubemapSize;
        spec.Height = m_ShadowCubemapSize;
        spec.Attachments.Attachments.push_back({ FBOTextureFormat::DEPTH_CUBEMAP });

        m_CubemapFBOs[lightIndex] = std::make_shared<FrameBuffer>(spec);
    }

    return m_CubemapFBOs[lightIndex];
}

void PointShadowMappingPass::RenderToCubemap(
    RenderContext &context,
    const std::array<glm::mat4, 6> &viewMatrices,
    const glm::mat4 &projectionMatrix,
    const glm::vec3 &lightPos,
    size_t lightIndex)
{
    // Attach ENTIRE cubemap to framebuffer (not a specific face)
    // Geometry shader will use gl_Layer to select the face
    auto fbo = m_CubemapFBOs[lightIndex];
    fbo->Bind();

    // Attach all 6 faces using glFramebufferTexture (not glFramebufferTexture2D)
    // Note: This is redundant with FrameBuffer::Invalidate() but ensures correct attachment
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        fbo->GetDepthCubemapID(), 0);

    // Verify framebuffer is complete
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        spdlog::error("Point shadow FBO incomplete! Status: 0x{:x}", status);
        return;
    }

    // Set viewport for cubemap
    glViewport(0, 0, m_ShadowCubemapSize, m_ShadowCubemapSize);

    // Enable depth testing for shadow map rendering
    RenderCommands::SetDepthTestData depthTestCmd{
        true,           // Enable depth test
        GL_LESS,        // Depth function
        true            // Enable depth write
    };
    Submit(depthTestCmd);

    // Enable back-face culling to optimize shadow rendering
    RenderCommands::SetFaceCullingData cullCmd{
        true,           // Enable culling
        GL_BACK         // Cull back faces
    };
    Submit(cullCmd);

    // Clear depth buffer to 1.0 (far plane)
    glClearDepth(1.0f);
    RenderCommands::ClearData clearCmd{
        0.0f, 0.0f, 0.0f, 1.0f,
        false,  // Don't clear color
        true    // Clear depth
    };
    Submit(clearCmd);

    // Bind depth shader (with geometry shader)
    Submit(RenderCommands::BindShaderData{ m_PointShadowDepthShader });

    // Pass all 6 shadow matrices to geometry shader
    m_PointShadowDepthShader->use();
    for (int i = 0; i < 6; ++i)
    {
        std::string matrixName = "u_ShadowMatrices[" + std::to_string(i) + "]";
        m_PointShadowDepthShader->setMat4(matrixName, projectionMatrix * viewMatrices[i]);
    }

    // Pass light position and far plane to fragment shader
    m_PointShadowDepthShader->setVec3("u_LightPos", lightPos);
    m_PointShadowDepthShader->setFloat("u_FarPlane", m_ShadowFarPlane);

    // Render all visible objects ONCE (geometry shader emits to all 6 faces)
    int objectCount = 0;
    spdlog::info("Point shadow pass: Total renderables in context = {}", context.renderables.size());

    for (const auto &renderable : context.renderables)
    {
        if (!renderable.visible) {
            spdlog::debug("Skipping invisible renderable");
            continue;
        }
        if (!renderable.mesh) {
            spdlog::debug("Skipping renderable with no mesh");
            continue;
        }

        // Set model matrix uniform
        m_PointShadowDepthShader->setMat4("u_Model", renderable.transform);

        // Draw (geometry shader will emit 6 triangles per input triangle, one per face)
        Submit(RenderCommands::DrawElementsData{
            renderable.mesh->GetVertexArray()->GetVAOHandle(),
            renderable.mesh->GetIndexCount(),
            GL_TRIANGLES
            });
        objectCount++;
    }
    spdlog::info("Point shadow light {}: Rendered {} out of {} objects",
                 lightIndex, objectCount, context.renderables.size());

    ExecuteCommands();
    fbo->Unbind();
}