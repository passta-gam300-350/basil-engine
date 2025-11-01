#include "../../include/Pipeline/PointShadowMappingPass.h"
#include "../../include/Pipeline/RenderContext.h"
#include "../../include/Core/RenderCommandBuffer.h"
#include "../../include/Utility/Light.h"
#include "../../include/Resources/Shader.h"
#include "../../include/Rendering/InstancedRenderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

PointShadowMappingPass::PointShadowMappingPass()
    : RenderPass("PointShadowPass"),  // No base FBO - we manage cubemap FBOs
      m_PointShadowShader(nullptr)
{
    InitializeCubemapFBOs();
    spdlog::info("PointShadowMappingPass: Initialized with {} cubemap FBOs (size: {}x{})",
                m_MaxPointLights, m_ShadowCubemapSize, m_ShadowCubemapSize);
}

PointShadowMappingPass::PointShadowMappingPass(std::shared_ptr<Shader> pointShadowShader)
    : RenderPass("PointShadowPass"),
      m_PointShadowShader(pointShadowShader)
{
    InitializeCubemapFBOs();
    spdlog::info("PointShadowMappingPass: Initialized with shader and {} cubemap FBOs",
                m_MaxPointLights);
}

PointShadowMappingPass::PointShadowMappingPass(std::shared_ptr<Shader> pointShadowShader,
                                               uint32_t shadowCubemapSize,
                                               uint32_t maxPointLights)
    : RenderPass("PointShadowPass"),
      m_PointShadowShader(pointShadowShader),
      m_ShadowCubemapSize(shadowCubemapSize),
      m_MaxPointLights(maxPointLights)
{
    InitializeCubemapFBOs();
    spdlog::info("PointShadowMappingPass: Initialized with custom settings ({} FBOs, size: {}x{})",
                m_MaxPointLights, m_ShadowCubemapSize, m_ShadowCubemapSize);
}

void PointShadowMappingPass::InitializeCubemapFBOs()
{
    if (m_CubemapsInitialized) {
        spdlog::warn("PointShadowMappingPass: Cubemaps already initialized");
        return;
    }

    // Pre-allocate cubemap FBOs for maximum number of point lights
    m_ShadowCubemaps.reserve(m_MaxPointLights);
    for (uint32_t i = 0; i < m_MaxPointLights; ++i) {
        m_ShadowCubemaps.push_back(
            std::make_shared<CubemapFrameBuffer>(m_ShadowCubemapSize)
        );
    }

    m_CubemapsInitialized = true;
}

void PointShadowMappingPass::SetShadowCubemapSize(uint32_t size)
{
    if (m_CubemapsInitialized) {
        spdlog::warn("PointShadowMappingPass: Cannot change cubemap size after initialization");
        return;
    }
    m_ShadowCubemapSize = size;
}

void PointShadowMappingPass::SetMaxPointLights(uint32_t maxLights)
{
    if (m_CubemapsInitialized) {
        spdlog::warn("PointShadowMappingPass: Cannot change max point lights after initialization");
        return;
    }
    m_MaxPointLights = maxLights;
}

void PointShadowMappingPass::Execute(RenderContext& context)
{
    if (!m_PointShadowShader) {
        spdlog::warn("PointShadowMappingPass: No shader set, skipping");
        return;
    }

    if (!m_CubemapsInitialized) {
        spdlog::error("PointShadowMappingPass: Cubemaps not initialized!");
        return;
    }

    // Find all point lights with shadows enabled
    std::vector<const SubmittedLightData*> pointLights;
    for (const auto& light : context.lights) {
        if (light.enabled && light.castShadows && light.type == Light::Type::Point) {
            pointLights.push_back(&light);
        }
    }

    if (pointLights.empty()) {
        // No point lights with shadows enabled - return (frame data is already cleared)
        return;
    }

    // Limit to max point lights (number of pre-allocated FBOs)
    size_t numLights = std::min(pointLights.size(), static_cast<size_t>(m_MaxPointLights));

    // Setup command buffer with systems from context
    SetupCommandBuffer(context);

    // Render shadow cubemap for each point light using pre-allocated FBOs
    for (size_t i = 0; i < numLights; ++i) {
        RenderPointShadowCubemap(context, *pointLights[i], i);
    }
}

void PointShadowMappingPass::RenderPointShadowCubemap(RenderContext& context,
                                                      const SubmittedLightData& light,
                                                      size_t lightIndex)
{
    // Use pre-allocated cubemap FBO
    auto& cubemapFBO = m_ShadowCubemaps[lightIndex];

    // Calculate shadow matrices for all 6 faces
    auto shadowMatrices = CalculateShadowMatrices(light.position, m_ShadowFarPlane);

    // === BEGIN COMMAND-BASED RENDERING ===

    // Clear commands for this light's rendering
    ClearCommands();

    // Bind cubemap FBO (using RAII-style binding)
    cubemapFBO->Bind();

    // Submit clear command
    RenderCommands::ClearData clearCmd{
        0.0f, 0.0f, 0.0f, 1.0f,  // color (not used)
        false,                    // don't clear color
        true                      // clear depth
    };
    Submit(clearCmd);

    // Render scene geometry using commands
    if (!context.renderables.empty()) {
        // Submit shadow matrices array command (6 matrices for cubemap faces)
        RenderCommands::SetUniformMat4ArrayData shadowMatricesCmd{
            m_PointShadowShader,
            "u_ShadowMatrices",
            shadowMatrices
        };
        Submit(shadowMatricesCmd);

        // Submit light position command
        RenderCommands::SetUniformVec3Data lightPosCmd{
            m_PointShadowShader,
            "u_LightPos",
            light.position
        };
        Submit(lightPosCmd);

        // Submit far plane command
        RenderCommands::SetUniformFloatData farPlaneCmd{
            m_PointShadowShader,
            "u_FarPlane",
            m_ShadowFarPlane
        };
        Submit(farPlaneCmd);

        // Use InstancedRenderer to render all objects with instancing
        context.instancedRenderer.RenderShadowToPass(
            *this,                      // This render pass
            context.renderables,        // All visible renderables
            m_PointShadowShader         // Instanced point shadow shader
        );
    }

    // Execute all submitted commands
    ExecuteCommands();

    // Unbind FBO (RAII-style)
    cubemapFBO->Unbind();

    // === END COMMAND-BASED RENDERING ===

    // Add point shadow to the unified shadow data array
    ShadowData pointShadow;
    pointShadow.shadowType = ShadowData::Point;
    pointShadow.lightSpaceMatrix = glm::mat4(1.0f);  // Not used for point lights
    pointShadow.textureIndex = static_cast<int32_t>(context.frameData.shadowCubemapTextures.size());
    pointShadow.farPlane = m_ShadowFarPlane;  // Used for depth normalization
    pointShadow.intensity = 0.8f;  // Default shadow intensity

    // Store shadow data and cubemap texture ID
    context.frameData.shadowDataArray.push_back(pointShadow);
    context.frameData.shadowCubemapTextures.push_back(cubemapFBO->GetCubemapTextureID());
}

std::vector<glm::mat4> PointShadowMappingPass::CalculateShadowMatrices(const glm::vec3& lightPos, float farPlane)
{
    // Projection matrix (90 degree FOV for cubemap faces)
    float aspect = 1.0f;
    float near = 0.1f;
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, farPlane);

    std::vector<glm::mat4> shadowMatrices;
    shadowMatrices.reserve(6);

    // View matrices for all 6 cubemap faces
    shadowMatrices.push_back(shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f))); // +X
    shadowMatrices.push_back(shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f))); // -X
    shadowMatrices.push_back(shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f))); // +Y
    shadowMatrices.push_back(shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f))); // -Y
    shadowMatrices.push_back(shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f))); // +Z
    shadowMatrices.push_back(shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))); // -Z

    return shadowMatrices;
}
