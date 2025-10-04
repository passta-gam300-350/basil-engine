/******************************************************************************/
/*!
\file   PBRLightingRenderer.cpp
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Implementation of physically-based rendering lighting system

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Rendering/PBRLightingRenderer.h"
#include "Scene/SceneRenderer.h"
#include "Utility/RenderData.h"
#include "Utility/Light.h"
#include "Resources/Material.h"
#include <spdlog/spdlog.h>
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

void PBRLightingRenderer::SetupPBRLighting(const std::shared_ptr<Shader>& shader,
                                           const FrameData& /*frameData*/,
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

    // Set material properties
    if (material != nullptr) {
        // Material properties are applied through the material's own methods
        // Material::ApplyPBRProperties() should be const-correct in Material class
        shader->setVec3("u_AlbedoColor", material->GetAlbedoColor());
        shader->setFloat("u_MetallicValue", material->GetMetallicValue());
        shader->setFloat("u_RoughnessValue", material->GetRoughnessValue());
    } else {
        SetupMaterialProperties(shader, material);
    }
}

void PBRLightingRenderer::SubmitLightingCommands(const std::shared_ptr<Shader>& shader,
                                                 const FrameData& frameData,
                                                 const Material* material)
{
    // For now, we'll use immediate setup since we don't have specific lighting commands yet
    // TODO: Add dedicated lighting commands to RenderCommandBuffer
    SetupPBRLighting(shader, frameData, material);
}

void PBRLightingRenderer::UpdateLighting(const std::vector<SubmittedLightData>& submittedLights,
                                         const glm::vec3& /*ambientLight*/, const FrameData& /*frameData*/)
{
    // Clear existing lights
    ClearLights();
    
    // Convert submitted light data to internal format
    for (const auto& submittedLight : submittedLights)
    {
        if (!submittedLight.enabled) {
            continue;
        }
            
        switch (submittedLight.type)
        {
            case Light::Type::Directional:
            {
                DirectionalLight dirLight{};
                dirLight.direction = submittedLight.direction;
                dirLight.color = submittedLight.color;
                dirLight.intensity = submittedLight.intensity;
                AddDirectionalLight(dirLight);
                break;
            }
            case Light::Type::Point:
            {
                PointLight pointLight{};
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
                SpotLight spotLight{};
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

void PBRLightingRenderer::ApplyLightingToShader(const std::shared_ptr<Shader>& shader, const Material* material)
{
    assert(shader && "Shader cannot be null for lighting application");
    assert(shader->ID != 0 && "Shader program must be compiled and linked");

    if (!shader) {
        spdlog::error("PBRLightingRenderer::ApplyLightingToShader: NULL shader provided");
        return;
    }

    // Ensure shader is active for uniform setting
    shader->use();

    // Set light data
    SetupPointLights(shader);
    SetupDirectionalLights(shader);
    SetupSpotLights(shader);

    // Set material properties
    if (material != nullptr) {
        // Material properties are applied through the material's own methods
        // Material::ApplyPBRProperties() should be const-correct in Material class
        shader->setVec3("u_AlbedoColor", material->GetAlbedoColor());
        shader->setFloat("u_MetallicValue", material->GetMetallicValue());
        shader->setFloat("u_RoughnessValue", material->GetRoughnessValue());
    } else {
        SetupMaterialProperties(shader, material);
    }
}

void PBRLightingRenderer::SetupPointLights(const std::shared_ptr<Shader>& shader)
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

void PBRLightingRenderer::SetupDirectionalLights(const std::shared_ptr<Shader>& shader)
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

void PBRLightingRenderer::SetupSpotLights(const std::shared_ptr<Shader>& shader)
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

void PBRLightingRenderer::SetupMaterialProperties(const std::shared_ptr<Shader>& shader, const Material* material)
{
    if (material == nullptr) {
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