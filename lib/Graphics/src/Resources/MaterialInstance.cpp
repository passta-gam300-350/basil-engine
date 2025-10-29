/******************************************************************************/
/*!
\file   MaterialInstance.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2025/10/20
\brief  Implementation of MaterialInstance for per-entity material customization

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#include <Resources/MaterialInstance.h>
#include <spdlog/spdlog.h>

MaterialInstance::MaterialInstance(std::shared_ptr<Material> baseMaterial, const std::string& instanceName)
    : m_BaseMaterial(baseMaterial)
{
    if (!baseMaterial) {
        spdlog::error("MaterialInstance: Cannot create instance from null base material");
        throw std::invalid_argument("Base material cannot be null");
    }

    if (instanceName.empty()) {
        m_InstanceName = baseMaterial->GetName() + " (Instance)";
    } else {
        m_InstanceName = instanceName;
    }

    spdlog::debug("MaterialInstance: Created instance '{}' from base '{}'",
                  m_InstanceName, baseMaterial->GetName());
}

bool MaterialInstance::HasOverrides() const {
    return !m_OverriddenProperties.empty();
}

void MaterialInstance::ClearOverrides() {
    m_FloatOverrides.clear();
    m_Vec3Overrides.clear();
    m_Vec4Overrides.clear();
    m_Mat4Overrides.clear();
    m_TextureOverrides.clear();
    m_OverriddenProperties.clear();

    spdlog::debug("MaterialInstance: Cleared all overrides for '{}'", m_InstanceName);
}

// ===== Property Setters =====

void MaterialInstance::SetFloat(const std::string& name, float value) {
    m_FloatOverrides[name] = value;
    m_OverriddenProperties.insert(name);
}

void MaterialInstance::SetVec3(const std::string& name, const glm::vec3& value) {
    m_Vec3Overrides[name] = value;
    m_OverriddenProperties.insert(name);
}

void MaterialInstance::SetVec4(const std::string& name, const glm::vec4& value) {
    m_Vec4Overrides[name] = value;
    m_OverriddenProperties.insert(name);
}

void MaterialInstance::SetMat4(const std::string& name, const glm::mat4& value) {
    m_Mat4Overrides[name] = value;
    m_OverriddenProperties.insert(name);
}

void MaterialInstance::SetTexture(const std::string& name, std::shared_ptr<Texture> texture, int textureUnit) {
    m_TextureOverrides[name] = std::make_pair(texture, textureUnit);
    m_OverriddenProperties.insert(name);
}

// ===== Property Getters (With Fallback) =====

float MaterialInstance::GetFloat(const std::string& name, float defaultValue) const {
    // Check override first
    auto it = m_FloatOverrides.find(name);
    if (it != m_FloatOverrides.end()) {
        return it->second;
    }

    // Fall back to base material
    return m_BaseMaterial->GetFloatProperty(name, defaultValue);
}

glm::vec3 MaterialInstance::GetVec3(const std::string& name, const glm::vec3& defaultValue) const {
    // Check override first
    auto it = m_Vec3Overrides.find(name);
    if (it != m_Vec3Overrides.end()) {
        return it->second;
    }

    // Fall back to base material
    return m_BaseMaterial->GetVec3Property(name, defaultValue);
}

glm::vec4 MaterialInstance::GetVec4(const std::string& name, const glm::vec4& defaultValue) const {
    // Check override first
    auto it = m_Vec4Overrides.find(name);
    if (it != m_Vec4Overrides.end()) {
        return it->second;
    }

    // Fall back to base material
    return m_BaseMaterial->GetVec4Property(name, defaultValue);
}

glm::mat4 MaterialInstance::GetMat4(const std::string& name, const glm::mat4& defaultValue) const {
    // Check override first
    auto it = m_Mat4Overrides.find(name);
    if (it != m_Mat4Overrides.end()) {
        return it->second;
    }

    // Fall back to base material
    return m_BaseMaterial->GetMat4Property(name, defaultValue);
}

std::shared_ptr<Texture> MaterialInstance::GetTexture(const std::string& name) const {
    // Check override first
    auto it = m_TextureOverrides.find(name);
    if (it != m_TextureOverrides.end()) {
        return it->second.first;
    }

    // Fall back to base material
    return m_BaseMaterial->GetTexture(name);
}

bool MaterialInstance::IsPropertyOverridden(const std::string& name) const {
    return m_OverriddenProperties.count(name) > 0;
}

// ===== Rendering =====

void MaterialInstance::ApplyProperties() {
    // First, use the shader from the base material
    auto shader = m_BaseMaterial->GetShader();
    if (!shader) {
        spdlog::warn("MaterialInstance: No shader available for '{}'", m_InstanceName);
        return;
    }

    shader->use();

    // Apply base material properties first
    m_BaseMaterial->ApplyAllProperties();

    // Then apply overrides (which will override the base values)
    for (const auto& [name, value] : m_FloatOverrides) {
        shader->setFloat(name, value);
    }

    for (const auto& [name, value] : m_Vec3Overrides) {
        shader->setVec3(name, value);
    }

    for (const auto& [name, value] : m_Vec4Overrides) {
        shader->setVec4(name, value);
    }

    for (const auto& [name, value] : m_Mat4Overrides) {
        shader->setMat4(name, value);
    }

    for (const auto& [name, texturePair] : m_TextureOverrides) {
        const auto& [texture, unit] = texturePair;
        if (texture && texture->id != 0) {
            // Use explicit unit if provided, otherwise let Material's auto-assignment handle it
            int actualUnit = (unit >= 0) ? unit : 0; // Default to unit 0 if not specified
            glActiveTexture(GL_TEXTURE0 + actualUnit);
            glBindTexture(texture->target, texture->id);
            shader->setInt(name, actualUnit);
        }
    }
}
