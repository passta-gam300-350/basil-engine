/******************************************************************************/
/*!
\file   MaterialPropertyBlock.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/22
\brief  Declares the MaterialPropertyBlock class for lightweight per-object
        material property overrides without creating material instances.

        Unity-style MaterialPropertyBlock enables GPU instancing-friendly
        per-object customization by applying property overrides per-draw call.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <Resources/Shader.h>
#include <Resources/Texture.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <glm/glm.hpp>

/**
 * @class MaterialPropertyBlock
 * @brief Lightweight per-object material property overrides without creating instances
 *
 * MaterialPropertyBlock provides Unity-style per-object property customization that:
 * - Preserves GPU instancing (doesn't create material copies)
 * - Uses minimal memory (only stores overridden properties)
 * - Applied per-draw call, not stored in material
 * - No shader/material reference needed
 *
 * Use Cases:
 * - Per-object color tint on shared material (grass blades, particle systems)
 * - Per-object parameter variation (metallic, roughness tweaks)
 * - Dynamic property updates (health bars, damage effects)
 *
 * Comparison to MaterialInstance:
 * - MaterialPropertyBlock: Lightweight, preserves instancing, per-draw overhead
 * - MaterialInstance: Full copy, breaks instancing, good for persistent changes
 *
 * Example Usage:
 * @code
 * // Create property block (reusable across frames)
 * auto propBlock = std::make_shared<MaterialPropertyBlock>();
 * propBlock->SetVec3("u_AlbedoColor", glm::vec3(1.0f, 0.0f, 0.0f));  // Red tint
 * propBlock->SetFloat("u_Roughness", 0.8f);  // Override roughness
 *
 * // Apply during rendering (called per-draw)
 * material->ApplyAllProperties();       // Base material properties
 * propBlock->ApplyToShader(shader);     // Override with block properties
 * // Draw call here
 * @endcode
 */
class MaterialPropertyBlock
{
public:
    /**
     * @brief Construct an empty MaterialPropertyBlock
     */
    MaterialPropertyBlock() = default;

    /**
     * @brief Destructor
     */
    ~MaterialPropertyBlock() = default;

    // ========== PROPERTY SETTERS ==========

    /**
     * @brief Set a float property
     * @param name Shader uniform name
     * @param value Float value to set
     */
    void SetFloat(const std::string& name, float value);

    /**
     * @brief Set a vec3 property
     * @param name Shader uniform name
     * @param value Vec3 value to set
     */
    void SetVec3(const std::string& name, const glm::vec3& value);

    /**
     * @brief Set a vec4 property
     * @param name Shader uniform name
     * @param value Vec4 value to set
     */
    void SetVec4(const std::string& name, const glm::vec4& value);

    /**
     * @brief Set a mat4 property
     * @param name Shader uniform name
     * @param value Mat4 value to set
     */
    void SetMat4(const std::string& name, const glm::mat4& value);

    /**
     * @brief Set a texture property
     * @param name Shader uniform name (sampler name)
     * @param texture Texture to bind
     * @param textureUnit Optional texture unit (0-15), if -1 auto-assigned
     *
     * @note When textureUnit is -1, textures are auto-assigned to units sequentially
     *       starting from unit 0. Manual assignment recommended for consistency.
     */
    void SetTexture(const std::string& name, std::shared_ptr<Texture> texture, int textureUnit = -1);

    // ========== PROPERTY GETTERS ==========

    /**
     * @brief Try to get a float property
     * @param name Property name
     * @param outValue Output value (unchanged if property not found)
     * @return true if property exists and is a float
     */
    bool TryGetFloat(const std::string& name, float& outValue) const;

    /**
     * @brief Try to get a vec3 property
     * @param name Property name
     * @param outValue Output value (unchanged if property not found)
     * @return true if property exists and is a vec3
     */
    bool TryGetVec3(const std::string& name, glm::vec3& outValue) const;

    /**
     * @brief Try to get a vec4 property
     * @param name Property name
     * @param outValue Output value (unchanged if property not found)
     * @return true if property exists and is a vec4
     */
    bool TryGetVec4(const std::string& name, glm::vec4& outValue) const;

    /**
     * @brief Try to get a mat4 property
     * @param name Property name
     * @param outValue Output value (unchanged if property not found)
     * @return true if property exists and is a mat4
     */
    bool TryGetMat4(const std::string& name, glm::mat4& outValue) const;

    /**
     * @brief Try to get a texture property
     * @param name Property name
     * @param outTexture Output texture (unchanged if property not found)
     * @param outTextureUnit Output texture unit (unchanged if property not found)
     * @return true if property exists and is a texture
     */
    bool TryGetTexture(const std::string& name, std::shared_ptr<Texture>& outTexture, int& outTextureUnit) const;

    // ========== PROPERTY QUERIES ==========

    /**
     * @brief Check if a property exists in the block
     * @param name Property name
     * @return true if property is set
     */
    bool HasProperty(const std::string& name) const;

    /**
     * @brief Get the number of properties in the block
     * @return Property count
     */
    size_t GetPropertyCount() const { return m_Properties.size(); }

    /**
     * @brief Check if the block is empty
     * @return true if no properties set
     */
    bool IsEmpty() const { return m_Properties.empty(); }

    // ========== APPLICATION ==========

    /**
     * @brief Apply all properties to the given shader
     * @param shader Shader to apply properties to (must be bound with use())
     *
     * @note This method assumes the shader is already bound.
     *       Call shader->use() before calling this method.
     *
     * @warning This performs glUniform* calls, so it has per-frame overhead.
     *          However, it's still faster than creating material instances.
     */
    void ApplyToShader(std::shared_ptr<Shader> shader) const;

    // ========== UTILITY ==========

    /**
     * @brief Clear all properties from the block
     */
    void Clear();

    /**
     * @brief Copy all properties from another block
     * @param other Source block to copy from
     */
    void CopyFrom(const MaterialPropertyBlock& other);

private:
    // Property value storage using std::variant for type safety
    using PropertyValue = std::variant<
        float,
        glm::vec3,
        glm::vec4,
        glm::mat4,
        std::pair<std::shared_ptr<Texture>, int>  // Texture + texture unit
    >;

    // Property map: uniform name -> value
    std::unordered_map<std::string, PropertyValue> m_Properties;

    // Auto-assigned texture unit counter (for SetTexture with unit = -1)
    mutable int m_NextTextureUnit = 0;
};
