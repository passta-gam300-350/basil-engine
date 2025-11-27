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
     * @brief Get shader for debug line rendering (physics debug visualization)
     * @return Debug line shader, or nullptr if not loaded
     */
    std::shared_ptr<Shader> GetDebugLineShader() const { return m_DebugLineShader; }

    /**
     * @brief Get shader for point shadow mapping (with geometry shader)
     * @return Point shadow shader, or nullptr if not loaded
     */
    std::shared_ptr<Shader> GetPointShadowShader() const { return m_PointShadowShader; }

    /**
     * @brief Get shader for outline rendering
     * @return Outline shader, or nullptr if not loaded
     */
    std::shared_ptr<Shader> GetOutlineShader() const { return m_OutlineShader; }

    /**
     * @brief Get shader for skybox rendering
     * @return Skybox shader, or nullptr if not loaded
     */
    std::shared_ptr<Shader> GetSkyboxShader() const { return m_SkyboxShader; }

    /**
     * @brief Get compute shader for HDR luminance calculation
     * @return HDR compute shader, or nullptr if not loaded
     */
    std::shared_ptr<Shader> GetHDRComputeShader() const { return m_HDRComputeShader; }

    /**
     * @brief Get shader for HDR tone mapping
     * @return Tone mapping shader, or nullptr if not loaded
     */
    std::shared_ptr<Shader> GetToneMappingShader() const { return m_ToneMappingShader; }

    /**
     * @brief Get shader for editor resolve (gamma correction for ImGui display)
     * @return Editor resolve shader, or nullptr if not loaded
     */
    std::shared_ptr<Shader> GetEditorResolveShader() const { return m_EditorResolveShader; }

    std::shared_ptr<Shader> GetParticleShader() const { return m_ParticleShader; }

    /**
     * @brief Get shader for HUD/UI rendering in screen space
     * @return HUD shader, or nullptr if not loaded
     */
    std::shared_ptr<Shader> GetHUDShader() const { return m_HUDShader; }

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
     * @brief Load a shader with geometry stage
     * @param name Unique shader identifier
     * @param vertPath Path to vertex shader
     * @param fragPath Path to fragment shader
     * @param geomPath Path to geometry shader
     * @return Shader pointer, or nullptr on failure
     */
    std::shared_ptr<Shader> LoadWithGeometry(
        const std::string& name,
        const std::string& vertPath,
        const std::string& fragPath,
        const std::string& geomPath
    );

    /**
     * @brief Load a compute shader
     * @param name Unique shader identifier
     * @param compPath Path to compute shader
     * @return Shader pointer, or nullptr on failure
     */
    std::shared_ptr<Shader> LoadCompute(
        const std::string& name,
        const std::string& compPath
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
    std::shared_ptr<Shader> m_DebugLineShader;       // Debug line shader for physics visualization
    std::shared_ptr<Shader> m_PointShadowShader;     // Point shadow with geometry shader
    std::shared_ptr<Shader> m_OutlineShader;
    std::shared_ptr<Shader> m_SkyboxShader;
    std::shared_ptr<Shader> m_HDRComputeShader;      // Compute shader for luminance
    std::shared_ptr<Shader> m_ToneMappingShader;
    std::shared_ptr<Shader> m_EditorResolveShader;   // Editor resolve with gamma correction
    std::shared_ptr<Shader> m_ParticleShader;
    std::shared_ptr<Shader> m_HUDShader;             // HUD/UI screen-space rendering
    // Named shader cache for custom/additional shaders
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;

    // Load essential shaders (called by Initialize)
    bool LoadPBRShader();
    bool LoadShadowShader();
    bool LoadPickingShader();
    bool LoadPrimitiveShader();
    bool LoadDebugLineShader();
    bool LoadPointShadowShader();
    bool LoadOutlineShader();
    bool LoadSkyboxShader();
    bool LoadHDRComputeShader();
    bool LoadToneMappingShader();
    bool LoadEditorResolveShader();
    bool LoadParticleShader();
    bool LoadHUDShader();
};

#endif // ENGINE_SHADER_LIBRARY_HPP
