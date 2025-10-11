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
{
    spdlog::info("PBRLightingRenderer: Initialized as lighting system");
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

void PBRLightingRenderer::SetupPBRLighting(std::shared_ptr<Shader> shader,
                                           const FrameData& frameData,
                                           const Material* material)
{
    assert(shader && "Shader cannot be null for PBR lighting setup");
    assert(shader->ID != 0 && "Shader program must be compiled and linked");

    if (!shader) {
        spdlog::error("PBRLightingRenderer::SetupPBRLighting: NULL shader provided");
        return;
    }

    // Ensure shader is active for uniform setting
    shader->use();

    // Set light counts
    SetupPointLights(shader);
    SetupDirectionalLights(shader);
    SetupSpotLights(shader);

    // Set material properties - use Material's method if available, else use legacy method
    if (material) {
        const_cast<Material*>(material)->ApplyPBRProperties();
    } else {
        SetupMaterialProperties(shader, material);
    }
}

void PBRLightingRenderer::SubmitLightingCommands(std::shared_ptr<Shader> shader,
                                                 const FrameData& frameData,
                                                 const Material* material)
{
    // For now, we'll use immediate setup since we don't have specific lighting commands yet
    // TODO: Add dedicated lighting commands to RenderCommandBuffer
    SetupPBRLighting(shader, frameData, material);
}

void PBRLightingRenderer::UpdateLighting(const std::vector<SubmittedLightData>& submittedLights,
                                         const glm::vec3& ambientLight, const FrameData& frameData)
{
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
                dirLight.intensity = submittedLight.intensity;
                AddDirectionalLight(dirLight);
                break;
            }
            case Light::Type::Point:
            {
                PointLight pointLight;
                pointLight.position = submittedLight.position;
                pointLight.color = submittedLight.color;
                pointLight.intensity = submittedLight.intensity;
                // Default attenuation values - could be configurable
                pointLight.constant = 1.0f;
                pointLight.linear = 0.09f;
                pointLight.quadratic = 0.032f;
                AddPointLight(pointLight);
                break;
            }
            case Light::Type::Spot:
            {
                SpotLight spotLight;
                spotLight.position = submittedLight.position;
                spotLight.direction = submittedLight.direction;
                spotLight.color = submittedLight.color;
                spotLight.intensity = submittedLight.intensity;
                spotLight.cutOff = glm::cos(glm::radians(submittedLight.innerCone));
                spotLight.outerCutOff = glm::cos(glm::radians(submittedLight.outerCone));
                // Default attenuation values
                spotLight.constant = 1.0f;
                spotLight.linear = 0.09f;
                spotLight.quadratic = 0.032f;
                AddSpotLight(spotLight);
                break;
            }
        }
    }
}

void PBRLightingRenderer::ApplyLightingToShader(std::shared_ptr<Shader> shader, const Material* material)
{
    assert(shader && "Shader cannot be null for lighting application");
    assert(shader->ID != 0 && "Shader program must be compiled and linked");

    if (!shader) {
        spdlog::error("PBRLightingRenderer::ApplyLightingToShader: NULL shader provided");
        return;
    }

    // Ensure shader is active for uniform setting
    shader->use();

    // Set ambient light
    shader->setVec3("u_AmbientLight", m_AmbientLight);

    // Set light data
    SetupPointLights(shader);
    SetupDirectionalLights(shader);
    SetupSpotLights(shader);

    // Set material properties - use Material's method if available, else use legacy method
    if (material) {
        const_cast<Material*>(material)->ApplyPBRProperties();
    } else {
        SetupMaterialProperties(shader, material);
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

void PBRLightingRenderer::SetupShadowMaps(std::shared_ptr<Shader> shader, const FrameData& frameData)
{
    assert(shader && "Shader cannot be null for shadow setup");
    assert(shader->ID != 0 && "Shader program must be compiled and linked");

    if (!shader) {
        spdlog::error("PBRLightingRenderer::SetupShadowMaps: NULL shader provided");
        return;
    }

    // Ensure shader is active
    shader->use();

    // Set shadow intensity parameters
    shader->setFloat("u_DirectionalShadowIntensity", m_DirectionalShadowIntensity);
    shader->setFloat("u_PointShadowIntensity", m_PointShadowIntensity);

    // Setup directional shadows
    SetupDirectionalShadows(shader, frameData);

    // Setup point shadows
    SetupPointShadows(shader, frameData);
}

void PBRLightingRenderer::SetupDirectionalShadows(std::shared_ptr<Shader> shader, const FrameData& frameData)
{
    // No directional shadow data available - disable shadow mapping
    if (frameData.shadowMaps.empty() || frameData.shadowMatrices.empty() || !frameData.shadowMaps[0]) {
        // Bind a default/dummy shadow matrix and unbind shadow texture
        glm::mat4 identityMatrix = glm::mat4(1.0f);
        shader->setMat4("u_LightSpaceMatrix", identityMatrix);
        shader->setBool("u_EnableShadows", false);
        return;
    }

    // Directional shadow data is available
    uint32_t shadowTexID = frameData.shadowMaps[0]->GetDepthAttachmentRendererID();
    shader->setMat4("u_LightSpaceMatrix", frameData.shadowMatrices[0]);
    shader->setBool("u_EnableShadows", true);

    // Bind shadow map texture to slot 8
    const int SHADOW_MAP_SLOT = 8;
    glActiveTexture(GL_TEXTURE0 + SHADOW_MAP_SLOT);
    glBindTexture(GL_TEXTURE_2D, shadowTexID);
    shader->setInt("u_ShadowMap", SHADOW_MAP_SLOT);
}

void PBRLightingRenderer::SetupPointShadows(std::shared_ptr<Shader> shader, const FrameData& frameData)
{
    // Always initialize ALL 4 samplerCube uniforms to prevent undefined behavior
    // Even unused samplers must be bound to valid texture units
    const int MAX_POINT_SHADOW_MAPS = 4;
    const int POINT_SHADOW_START_SLOT = 9;

    int activeShadowMaps = (!frameData.pointShadowCubemaps.empty() && !frameData.pointShadowFarPlanes.empty())
                           ? static_cast<int>(frameData.pointShadowCubemaps.size())
                           : 0;

    shader->setInt("u_NumPointShadowMaps", activeShadowMaps);

    for (int i = 0; i < MAX_POINT_SHADOW_MAPS; ++i) {
        uint32_t textureUnit = POINT_SHADOW_START_SLOT + static_cast<uint32_t>(i);
        glActiveTexture(GL_TEXTURE0 + textureUnit);

        if (i < activeShadowMaps) {
            // Bind actual shadow cubemap
            glBindTexture(GL_TEXTURE_CUBE_MAP, frameData.pointShadowCubemaps[i]);

            // Set far plane uniform
            std::string farPlaneName = "u_PointShadowFarPlanes[" + std::to_string(i) + "]";
            shader->setFloat(farPlaneName, frameData.pointShadowFarPlanes[i]);
        } else {
            // Bind dummy cubemap (texture ID 0) for unused slots
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        }

        // Always set sampler uniform to proper texture unit
        std::string samplerName = "u_PointShadowMaps[" + std::to_string(i) + "]";
        shader->setInt(samplerName, static_cast<int>(textureUnit));
    }
}

void PBRLightingRenderer::SetShadowIntensity(float directionalIntensity, float pointIntensity)
{
    m_DirectionalShadowIntensity = glm::clamp(directionalIntensity, 0.0f, 1.0f);
    m_PointShadowIntensity = glm::clamp(pointIntensity, 0.0f, 1.0f);
}