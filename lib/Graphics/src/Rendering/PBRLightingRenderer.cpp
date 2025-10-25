#include "Rendering/PBRLightingRenderer.h"
#include "Scene/SceneRenderer.h"
#include "Utility/RenderData.h"
#include "Utility/Light.h"
#include "Resources/Material.h"
#include <spdlog/spdlog.h>
#include <glad/glad.h>
#include <glm/gtc/constants.hpp>
#include <cassert>

PBRLightingRenderer::PBRLightingRenderer()
    : m_ShadowSSBO(std::make_unique<TypedShaderStorageBuffer<ShadowData>>())
{
    spdlog::info("PBRLightingRenderer: Initialized as lighting system with SSBO shadow support");
}

void PBRLightingRenderer::ClearLights()
{
    m_PointLights.clear();
    m_DirectionalLights.clear();
    m_SpotLights.clear();
}

void PBRLightingRenderer::AddPointLight(const PointLight& light)
{
    m_PointLights.push_back(light);
}

void PBRLightingRenderer::AddDirectionalLight(const DirectionalLight& light)
{
    m_DirectionalLights.push_back(light);
}

void PBRLightingRenderer::AddSpotLight(const SpotLight& light)
{
    m_SpotLights.push_back(light);
}

void PBRLightingRenderer::UpdateLighting(const std::vector<SubmittedLightData>& submittedLights,
                                         const glm::vec3& ambientLight, const FrameData& frameData)
{
    // PERFORMANCE: Reset lighting and shadow cache at start of each frame
    m_LastLightingShader.reset();
    m_LastShadowShader.reset();

    // Clear existing lights
    ClearLights();

    // Store ambient light
    m_AmbientLight = ambientLight;

    // Convert submitted light data to internal format
    for (const auto& submittedLight : submittedLights)
    {
        if (!submittedLight.enabled)
            continue;
            
        switch (submittedLight.type)
        {
            case Light::Type::Directional:
            {
                DirectionalLight dirLight;
                dirLight.direction = submittedLight.direction;
                dirLight.color = submittedLight.color;
                dirLight.intensity = submittedLight.diffuseIntensity;  // Use diffuse intensity
                dirLight.ambientIntensity = submittedLight.ambientIntensity;  // Per-light ambient
                AddDirectionalLight(dirLight);
                break;
            }
            case Light::Type::Point:
            {
                PointLight pointLight;
                pointLight.position = submittedLight.position;
                pointLight.color = submittedLight.color;
                pointLight.intensity = submittedLight.diffuseIntensity;  // Use diffuse intensity
                pointLight.ambientIntensity = submittedLight.ambientIntensity;  // Per-light ambient
                // Physically correct inverse-square attenuation for PBR (1/(1+d²) approximates 1/d²)
                pointLight.constant = 1.0f;
                pointLight.linear = 0.0f;
                pointLight.quadratic = 1.0f;
                AddPointLight(pointLight);
                break;
            }
            case Light::Type::Spot:
            {
                SpotLight spotLight;
                spotLight.position = submittedLight.position;
                spotLight.direction = submittedLight.direction;
                spotLight.color = submittedLight.color;
                spotLight.intensity = submittedLight.diffuseIntensity;  // Use diffuse intensity
                spotLight.ambientIntensity = submittedLight.ambientIntensity;  // Per-light ambient
                spotLight.cutOff = glm::cos(glm::radians(submittedLight.innerCone));
                spotLight.outerCutOff = glm::cos(glm::radians(submittedLight.outerCone));
                // Physically correct inverse-square attenuation for PBR (1/(1+d²) approximates 1/d²)
                spotLight.constant = 1.0f;
                spotLight.linear = 0.0f;
                spotLight.quadratic = 1.0f;
                AddSpotLight(spotLight);
                break;
            }
        }
    }
}

void PBRLightingRenderer::SetupPointLights(std::shared_ptr<Shader> shader)
{
    assert(shader && "Shader cannot be null for point light setup");
    assert(shader->ID != 0 && "Shader program must be compiled and linked");

    // Set point light count
    shader->setInt("u_NumPointLights", static_cast<int>(m_PointLights.size()));

    // Set up point lights
    for (size_t i = 0; i < m_PointLights.size(); ++i) {
        const auto& light = m_PointLights[i];
        assert(light.intensity >= 0.0f && "Point light intensity must be non-negative");
        assert(light.constant > 0.0f && "Point light constant attenuation must be positive");
        assert(light.linear >= 0.0f && "Point light linear attenuation must be non-negative");
        assert(light.quadratic >= 0.0f && "Point light quadratic attenuation must be non-negative");

        std::string prefix = "u_PointLights[" + std::to_string(i) + "].";
        shader->setVec3(prefix + "position", light.position);
        shader->setVec3(prefix + "color", light.color);
        shader->setFloat(prefix + "intensity", light.intensity);
        shader->setFloat(prefix + "ambientIntensity", light.ambientIntensity);  // Ogldev-style per-light ambient
        shader->setFloat(prefix + "constant", light.constant);
        shader->setFloat(prefix + "linear", light.linear);
        shader->setFloat(prefix + "quadratic", light.quadratic);
    }
}

void PBRLightingRenderer::SetupDirectionalLights(std::shared_ptr<Shader> shader)
{
    assert(shader && "Shader cannot be null for directional light setup");
    assert(shader->ID != 0 && "Shader program must be compiled and linked");

    // Set directional light count
    shader->setInt("u_NumDirectionalLights", static_cast<int>(m_DirectionalLights.size()));

    // Set up directional lights
    for (size_t i = 0; i < m_DirectionalLights.size(); ++i) {
        const auto& light = m_DirectionalLights[i];
        assert(light.intensity >= 0.0f && "Directional light intensity must be non-negative");
        assert(glm::length(light.direction) > 0.0f && "Directional light direction must be non-zero");

        std::string prefix = "u_DirectionalLights[" + std::to_string(i) + "].";
        shader->setVec3(prefix + "direction", light.direction);
        shader->setVec3(prefix + "color", light.color);
        shader->setFloat(prefix + "intensity", light.intensity);
        shader->setFloat(prefix + "ambientIntensity", light.ambientIntensity);  // Ogldev-style per-light ambient
    }
}

void PBRLightingRenderer::SetupSpotLights(std::shared_ptr<Shader> shader)
{
    // Set spot light count
    shader->setInt("u_NumSpotLights", static_cast<int>(m_SpotLights.size()));
    
    // Set up spot lights
    for (size_t i = 0; i < m_SpotLights.size(); ++i) {
        const auto& light = m_SpotLights[i];
        std::string prefix = "u_SpotLights[" + std::to_string(i) + "].";
        shader->setVec3(prefix + "position", light.position);
        shader->setVec3(prefix + "direction", light.direction);
        shader->setVec3(prefix + "color", light.color);
        shader->setFloat(prefix + "intensity", light.intensity);
        shader->setFloat(prefix + "ambientIntensity", light.ambientIntensity);  // Ogldev-style per-light ambient
        shader->setFloat(prefix + "cutOff", light.cutOff);
        shader->setFloat(prefix + "outerCutOff", light.outerCutOff);
        shader->setFloat(prefix + "constant", light.constant);
        shader->setFloat(prefix + "linear", light.linear);
        shader->setFloat(prefix + "quadratic", light.quadratic);
    }
}

void PBRLightingRenderer::SetupMaterialProperties(std::shared_ptr<Shader> shader, const Material* material)
{
    if (!material) {
        // Use default PBR properties if no material provided
        shader->setVec3("u_AlbedoColor", glm::vec3(0.8f, 0.7f, 0.6f));
        shader->setFloat("u_MetallicValue", 0.7f);
        shader->setFloat("u_RoughnessValue", 0.3f);
        return;
    }

    // Use Material class methods - only set material properties, not texture flags
    shader->setVec3("u_AlbedoColor", material->GetAlbedoColor());
    shader->setFloat("u_MetallicValue", material->GetMetallicValue());
    shader->setFloat("u_RoughnessValue", material->GetRoughnessValue());

    // Note: Bindless texture system handles u_HasDiffuseMap, u_HasNormalMap, etc.
    // and texture handle uploads to SSBO at binding point 1
}

void PBRLightingRenderer::SetShadowIntensity(float directionalIntensity, float pointIntensity)
{
    m_DirectionalShadowIntensity = glm::clamp(directionalIntensity, 0.0f, 1.0f);
    m_PointShadowIntensity = glm::clamp(pointIntensity, 0.0f, 1.0f);
}

// ===== OPTION A: COMMAND-BASED LIGHTING SETUP =====
#include "Pipeline/RenderPass.h"
#include "Core/RenderCommandBuffer.h"

void PBRLightingRenderer::SubmitLightingCommands(RenderPass& renderPass,
                                                   std::shared_ptr<Shader> shader,
                                                   const Material* material)
{
    assert(shader && "Shader cannot be null for lighting submission");
    assert(shader->ID != 0 && "Shader program must be compiled and linked");

    // PERFORMANCE: Skip if lighting already set up for this shader this frame
    if (m_LastLightingShader == shader) {
        return;
    }
    m_LastLightingShader = shader;

    // 1. Submit ambient light
    RenderCommands::SetUniformVec3Data ambientCmd{
        shader,
        "u_AmbientLight",
        m_AmbientLight
    };
    renderPass.Submit(ambientCmd);

    // 2. Convert and submit point lights
    std::vector<RenderCommands::PointLightData> pointLightData;
    pointLightData.reserve(m_PointLights.size());
    for (const auto& light : m_PointLights) {
        pointLightData.push_back({
            light.position,
            light.color,
            light.intensity,
            light.ambientIntensity,
            light.constant,
            light.linear,
            light.quadratic
        });
    }
    RenderCommands::SetPointLightsData pointLightsCmd{shader, pointLightData};
    renderPass.Submit(pointLightsCmd);

    // 3. Convert and submit directional lights
    std::vector<RenderCommands::DirectionalLightData> dirLightData;
    dirLightData.reserve(m_DirectionalLights.size());
    for (const auto& light : m_DirectionalLights) {
        dirLightData.push_back({
            light.direction,
            light.color,
            light.intensity,
            light.ambientIntensity
        });
    }
    RenderCommands::SetDirectionalLightsData dirLightsCmd{shader, dirLightData};
    renderPass.Submit(dirLightsCmd);

    // 4. Convert and submit spot lights
    std::vector<RenderCommands::SpotLightData> spotLightData;
    spotLightData.reserve(m_SpotLights.size());
    for (const auto& light : m_SpotLights) {
        spotLightData.push_back({
            light.position,
            light.direction,
            light.color,
            light.intensity,
            light.ambientIntensity,
            light.cutOff,
            light.outerCutOff,
            light.constant,
            light.linear,
            light.quadratic
        });
    }
    RenderCommands::SetSpotLightsData spotLightsCmd{shader, spotLightData};
    renderPass.Submit(spotLightsCmd);

    // 5. Submit material properties (if material provided)
    // Note: For instanced rendering, materials are per-instance in SSBO, so we skip this
    if (material) {
        RenderCommands::SetMaterialPBRData materialCmd{
            shader,
            material->GetAlbedoColor(),
            material->GetMetallicValue(),
            material->GetRoughnessValue()
        };
        renderPass.Submit(materialCmd);
    }
}

void PBRLightingRenderer::SubmitShadowCommands(RenderPass& renderPass,
                                                std::shared_ptr<Shader> shader,
                                                const FrameData& frameData)
{
    assert(shader && "Shader cannot be null for shadow submission");
    assert(shader->ID != 0 && "Shader program must be compiled and linked");

    // PERFORMANCE: Skip if shadows already set up for this shader this frame
    if (m_LastShadowShader == shader) {
        return;
    }
    m_LastShadowShader = shader;

    // UNIFIED SHADOW SYSTEM (SSBO + Texture Arrays)
    if (!frameData.shadowDataArray.empty()) {
        // Ensure buffer is large enough before uploading
        uint32_t requiredSize = static_cast<uint32_t>(frameData.shadowDataArray.size() * sizeof(ShadowData));
        if (m_ShadowSSBO->GetSize() < requiredSize) {
            // Allocate with some headroom to reduce reallocations
            m_ShadowSSBO->Resize(requiredSize * 2);
        }

        // Upload shadow data to GPU
        m_ShadowSSBO->SetData(frameData.shadowDataArray);

        // Bind SSBO to binding point 1 (matches shader layout)
        RenderCommands::BindSSBOData ssboCmd{
            m_ShadowSSBO->GetSSBOHandle(),
            1  // Binding point 1 for shadow data
        };
        renderPass.Submit(ssboCmd);

        // Set number of shadows
        RenderCommands::SetUniformIntData numShadowsCmd{
            shader,
            "u_NumShadows",
            static_cast<int>(frameData.shadowDataArray.size())
        };
        renderPass.Submit(numShadowsCmd);

        // Bind 2D shadow textures (directional/spot) to individual slots
        // TODO: Replace with sampler2DArray once implemented
        for (size_t i = 0; i < frameData.shadow2DTextures.size() && i < 16; ++i) {
            std::string uniformName = "u_Shadow2DTextures[" + std::to_string(i) + "]";
            renderPass.Submit(RenderCommands::BindTextureIDData{
                frameData.shadow2DTextures[i],
                static_cast<uint32_t>(8 + i),  // Texture units 8-23
                shader,
                uniformName
            });
        }

        // Bind cubemap shadow textures (point) to individual slots
        // TODO: Replace with samplerCubeArray once implemented
        for (size_t i = 0; i < frameData.shadowCubemapTextures.size() && i < 8; ++i) {
            std::string uniformName = "u_ShadowCubemapTextures[" + std::to_string(i) + "]";
            renderPass.Submit(RenderCommands::BindCubemapData{
                frameData.shadowCubemapTextures[i],
                static_cast<uint32_t>(24 + i),  // Texture units 24-31
                shader,
                uniformName
            });
        }

        spdlog::debug("PBRLightingRenderer: Uploaded {} shadows ({} 2D, {} cubemap) to SSBO",
                      frameData.shadowDataArray.size(),
                      frameData.shadow2DTextures.size(),
                      frameData.shadowCubemapTextures.size());
    } else {
        // No shadows - set count to 0
        RenderCommands::SetUniformIntData numShadowsCmd{
            shader,
            "u_NumShadows",
            0
        };
        renderPass.Submit(numShadowsCmd);
    }
}