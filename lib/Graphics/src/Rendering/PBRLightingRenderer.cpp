#include "Rendering/PBRLightingRenderer.h"
#include "Scene/SceneRenderer.h"
#include "Utility/RenderData.h"
#include "Utility/Light.h"
#include <iostream>

// Static instance for scene-wide access
PBRLightingRenderer* PBRLightingRenderer::s_Instance = nullptr;

PBRLightingRenderer::PBRLightingRenderer()
{
    // Set static instance for global access
    s_Instance = this;
    std::cout << "PBRLightingRenderer: Initialized as scene-wide lighting system" << std::endl;
}

PBRLightingRenderer* PBRLightingRenderer::GetInstance()
{
    return s_Instance;
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
                                           const PBRMaterialProperties& material)
{
    if (!shader) {
        std::cerr << "PBRLightingRenderer::SetupPBRLighting: NULL shader provided" << std::endl;
        return;
    }

    // Ensure shader is active for uniform setting
    shader->use();
    
    // Set light counts
    SetupPointLights(shader);
    SetupDirectionalLights(shader);
    SetupSpotLights(shader);
    
    // Set material properties
    SetupMaterialProperties(shader, material);
}

void PBRLightingRenderer::SubmitLightingCommands(std::shared_ptr<Shader> shader, 
                                                 const FrameData& frameData, 
                                                 const PBRMaterialProperties& material,
                                                 const RenderCommands::CommandSortKey& sortKey)
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

void PBRLightingRenderer::ApplyLightingToShader(std::shared_ptr<Shader> shader, const PBRMaterialProperties& material)
{
    if (!shader) {
        std::cerr << "PBRLightingRenderer::ApplyLightingToShader: NULL shader provided" << std::endl;
        return;
    }

    // Ensure shader is active for uniform setting
    shader->use();
    
    // Set light data
    SetupPointLights(shader);
    SetupDirectionalLights(shader);
    SetupSpotLights(shader);
    
    // Set material properties
    SetupMaterialProperties(shader, material);
}

void PBRLightingRenderer::SetupPointLights(std::shared_ptr<Shader> shader)
{
    // Set point light count
    shader->setInt("u_NumPointLights", static_cast<int>(m_PointLights.size()));
    
    // Set up point lights
    for (size_t i = 0; i < m_PointLights.size(); ++i) {
        const auto& light = m_PointLights[i];
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
    // Set directional light count
    shader->setInt("u_NumDirectionalLights", static_cast<int>(m_DirectionalLights.size()));
    
    // Set up directional lights
    for (size_t i = 0; i < m_DirectionalLights.size(); ++i) {
        const auto& light = m_DirectionalLights[i];
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

void PBRLightingRenderer::SetupMaterialProperties(std::shared_ptr<Shader> shader, const PBRMaterialProperties& material)
{
    // Set PBR material properties
    shader->setVec3("u_AlbedoColor", material.albedoColor);
    shader->setFloat("u_MetallicValue", material.metallicValue);
    shader->setFloat("u_RoughnessValue", material.roughnessValue);
    
    // Set texture availability flag
    shader->setBool("u_HasTexture0", material.hasTexture);
    if (material.hasTexture) {
        shader->setInt("u_Texture0", 0);  // Texture unit 0
    }
}