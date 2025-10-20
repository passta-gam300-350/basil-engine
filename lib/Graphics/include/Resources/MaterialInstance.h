/******************************************************************************/
/*!
\file   MaterialInstance.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2025/10/20
\brief  Material Instance system for per-entity material customization.

        Implements Unity-style material instances where modifying a material
        on an entity creates a local copy that doesn't affect other entities.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <Resources/Material.h>
#include <memory>
#include <string>
#include <unordered_set>

/**
 * @brief Material instance that inherits from a base material
 *
 * When an entity modifies a material property, a MaterialInstance is automatically
 * created. The instance stores only overridden properties and falls back to the
 * base material for non-overridden values.
 *
 * This follows Unity's approach:
 * - Renderer.sharedMaterial: Returns the shared asset (changes affect all entities)
 * - Renderer.material: Returns/creates instance (changes are per-entity)
 */
class MaterialInstance {
public:
    /**
     * @brief Create a material instance from a base material
     * @param baseMaterial The shared material to instance from
     * @param instanceName Optional name for the instance (defaults to "BaseName (Instance)")
     */
    explicit MaterialInstance(std::shared_ptr<Material> baseMaterial, const std::string& instanceName = "");

    ~MaterialInstance() = default;

    // ===== Accessors =====

    /**
     * @brief Get the base material this instance references
     */
    std::shared_ptr<Material> GetBaseMaterial() const { return m_BaseMaterial; }

    /**
     * @brief Get the shader (always from base material)
     */
    std::shared_ptr<Shader> GetShader() const { return m_BaseMaterial->GetShader(); }

    /**
     * @brief Get instance name
     */
    const std::string& GetName() const { return m_InstanceName; }

    /**
     * @brief Check if this instance has any overrides
     */
    bool HasOverrides() const;

    /**
     * @brief Reset all overrides (reverts to base material)
     */
    void ClearOverrides();

    // ===== Property Setters (Create Overrides) =====

    /**
     * @brief Set a float property (creates override)
     */
    void SetFloat(const std::string& name, float value);

    /**
     * @brief Set a vec3 property (creates override)
     */
    void SetVec3(const std::string& name, const glm::vec3& value);

    /**
     * @brief Set a vec4 property (creates override)
     */
    void SetVec4(const std::string& name, const glm::vec4& value);

    /**
     * @brief Set a mat4 property (creates override)
     */
    void SetMat4(const std::string& name, const glm::mat4& value);

    // ===== Property Getters (With Fallback) =====

    /**
     * @brief Get float property (checks override first, then base material)
     */
    float GetFloat(const std::string& name, float defaultValue = 0.0f) const;

    /**
     * @brief Get vec3 property (checks override first, then base material)
     */
    glm::vec3 GetVec3(const std::string& name, const glm::vec3& defaultValue = glm::vec3(0.0f)) const;

    /**
     * @brief Get vec4 property (checks override first, then base material)
     */
    glm::vec4 GetVec4(const std::string& name, const glm::vec4& defaultValue = glm::vec4(0.0f)) const;

    /**
     * @brief Get mat4 property (checks override first, then base material)
     */
    glm::mat4 GetMat4(const std::string& name, const glm::mat4& defaultValue = glm::mat4(1.0f)) const;

    /**
     * @brief Check if a property is overridden locally
     */
    bool IsPropertyOverridden(const std::string& name) const;

    // ===== Rendering =====

    /**
     * @brief Apply all properties to shader (overrides + base material properties)
     * @note This should be called before rendering with this material
     */
    void ApplyProperties();

private:
    std::shared_ptr<Material> m_BaseMaterial;  ///< The shared material we instance from
    std::string m_InstanceName;                 ///< Display name for this instance

    // Override storage - only stores properties that differ from base
    std::unordered_map<std::string, float> m_FloatOverrides;
    std::unordered_map<std::string, glm::vec3> m_Vec3Overrides;
    std::unordered_map<std::string, glm::vec4> m_Vec4Overrides;
    std::unordered_map<std::string, glm::mat4> m_Mat4Overrides;

    // Track which properties are overridden (for efficient queries)
    std::unordered_set<std::string> m_OverriddenProperties;
};
