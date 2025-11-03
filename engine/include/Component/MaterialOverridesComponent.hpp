/******************************************************************************/
/*!
\file   MaterialOverridesComponent.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2025/10/31
\brief  Component for storing per-entity material property overrides.

        This component stores serializable material property overrides that are
        applied via MaterialInstance at runtime. Follows Unity's pattern of
        separating mesh rendering from material customization.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef ENGINE_MATERIALOVERRIDESCOMPONENT_HPP
#define ENGINE_MATERIALOVERRIDESCOMPONENT_HPP

#include <unordered_map>
#include <string>
#include <glm/glm.hpp>
#include <native/native.h>

/**
 * @brief Component for per-entity material property overrides
 *
 * Stores serializable material property overrides that differ from the base material.
 * MaterialOverridesSystem creates MaterialPropertyBlocks from this data at runtime.
 *
 * Usage:
 * - Override float properties: floatOverrides["u_MetallicValue"] = 0.8f
 * - Override vec3 properties: vec3Overrides["u_AlbedoColor"] = glm::vec3(1,0,0)
 * - Override textures: textureOverrides["u_AlbedoMap"] = someTextureGuid
 *
 * Property names must match shader uniform names (e.g., "u_MetallicValue" not "metallic").
 *
 * This follows Unity's MaterialPropertyBlock approach:
 * - MeshRendererComponent: References base material (shared)
 * - MaterialOverridesComponent: Stores per-entity customizations (serialized)
 * - MaterialPropertyBlock: Runtime representation (created by MaterialOverridesSystem)
 *
 * @note Uses MaterialPropertyBlock (not MaterialInstance) to preserve GPU instancing.
 */
struct MaterialOverridesComponent {
    /// Float property overrides (e.g., u_MetallicValue, u_RoughnessValue)
    std::unordered_map<std::string, float> floatOverrides;

    /// Vec3 property overrides (e.g., u_AlbedoColor, u_EmissionColor)
    std::unordered_map<std::string, glm::vec3> vec3Overrides;

    /// Vec4 property overrides (e.g., u_CustomColor)
    std::unordered_map<std::string, glm::vec4> vec4Overrides;

    /// Mat4 property overrides (e.g., u_CustomTransform)
    std::unordered_map<std::string, glm::mat4> mat4Overrides;

    /// Texture property overrides (stores asset GUIDs, not loaded textures)
    /// Maps uniform name (e.g., "u_AlbedoMap") to texture asset GUID
    std::unordered_map<std::string, rp::Guid> textureOverrides;

    /**
     * @brief Check if this component has any overrides
     * @return True if at least one property is overridden
     */
    bool HasOverrides() const {
        return !floatOverrides.empty() ||
               !vec3Overrides.empty() ||
               !vec4Overrides.empty() ||
               !mat4Overrides.empty() ||
               !textureOverrides.empty();
    }

    /**
     * @brief Clear all property overrides
     */
    void ClearAll() {
        floatOverrides.clear();
        vec3Overrides.clear();
        vec4Overrides.clear();
        mat4Overrides.clear();
        textureOverrides.clear();
    }
};

#endif // ENGINE_MATERIALOVERRIDESCOMPONENT_HPP
