#ifndef ENGINE_SHADER_LIBRARY_HPP
#define ENGINE_SHADER_LIBRARY_HPP
#pragma once

#include <Resources/Shader.h>
#include <memory>
#include <unordered_map>
#include <string>

// Forward declaration
class ResourceManager;

/**
 * @brief Manages shader loading and caching for the render system
 *
 * Responsibilities:
 * - Load shaders from disk via ResourceManager
 * - Cache loaded shaders by name
 * - Provide typed access to common shaders (PBR, shadow, picking, etc.)
 *
 * Lifetime: Owned by RenderSystem
 */
class ShaderLibrary {
public:
    /**
     * @brief Construct shader library with ResourceManager
     * @param resourceManager Pointer to graphics resource manager (must not be null)
     */
    explicit ShaderLibrary(ResourceManager* resourceManager);
    ~ShaderLibrary() = default;

    // Delete copy/move to prevent accidental duplication
    ShaderLibrary(const ShaderLibrary&) = delete;
    ShaderLibrary& operator=(const ShaderLibrary&) = delete;
    ShaderLibrary(ShaderLibrary&&) = delete;
    ShaderLibrary& operator=(ShaderLibrary&&) = delete;

    /**
     * @brief Initialize and load essential shaders
     * @return true if all essential shaders loaded successfully
     */
    bool Initialize();

    /**
     * @brief Get the main PBR shader for scene rendering
     * @return PBR shader, or nullptr if not loaded
     */
    std::shared_ptr<Shader> GetPBRShader() const { return m_PBRShader; }

    /**
     * @brief Get shader for shadow depth rendering
     * @return Shadow shader, or nullptr if not loaded
     */
    std::shared_ptr<Shader> GetShadowShader() const { return m_ShadowShader; }

    /**
     * @brief Get shader for object picking
     * @return Picking shader, or nullptr if not loaded
     */
    std::shared_ptr<Shader> GetPickingShader() const { return m_PickingShader; }

    /**
     * @brief Get shader for debug primitive rendering
     * @return Primitive shader, or nullptr if not loaded
     */
    std::shared_ptr<Shader> GetPrimitiveShader() const { return m_PrimitiveShader; }

    /**
     * @brief Load a shader by name
     * @param name Unique shader identifier
     * @param vertPath Path to vertex shader
     * @param fragPath Path to fragment shader
     * @return Shader pointer, or nullptr on failure
     */
    std::shared_ptr<Shader> Load(
        const std::string& name,
        const std::string& vertPath,
        const std::string& fragPath
    );

    /**
     * @brief Get a previously loaded shader by name
     * @param name Shader identifier
     * @return Shader pointer, or nullptr if not found
     */
    std::shared_ptr<Shader> Get(const std::string& name) const;

    /**
     * @brief Check if shader exists in library
     * @param name Shader identifier
     * @return true if shader is loaded
     */
    bool Has(const std::string& name) const;

    /**
     * @brief Clear all cached shaders
     */
    void Clear();

private:
    ResourceManager* m_ResourceManager;

    // Essential shaders for engine rendering
    std::shared_ptr<Shader> m_PBRShader;
    std::shared_ptr<Shader> m_ShadowShader;
    std::shared_ptr<Shader> m_PickingShader;
    std::shared_ptr<Shader> m_PrimitiveShader;

    // Named shader cache for custom/additional shaders
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;

    // Load essential shaders (called by Initialize)
    bool LoadPBRShader();
    bool LoadShadowShader();
    bool LoadPickingShader();
    bool LoadPrimitiveShader();
};

#endif // ENGINE_SHADER_LIBRARY_HPP
