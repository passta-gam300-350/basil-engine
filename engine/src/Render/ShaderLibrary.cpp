/******************************************************************************/
/*!
\file   ShaderLibrary.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Shader library implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Render/ShaderLibrary.hpp"
#include <Resources/ResourceManager.h>
#include <spdlog/spdlog.h>
#include <cassert>

ShaderLibrary::ShaderLibrary(ResourceManager* resourceManager)
    : m_ResourceManager(resourceManager)
{
    assert(resourceManager && "ResourceManager cannot be null");
}

bool ShaderLibrary::Initialize() {
    spdlog::info("ShaderLibrary: Initializing...");

    bool success = true;

    // Load essential shaders
    // PBR shader is critical, others are optional but recommended
    success &= LoadPBRShader();

    // These are optional - log warnings but don't fail initialization
    if (!LoadShadowShader()) {
        spdlog::warn("ShaderLibrary: Shadow shader not loaded - shadows will be disabled");
    }
    if (!LoadPointShadowShader()) {
        spdlog::warn("ShaderLibrary: Point shadow shader not loaded - point light shadows will be disabled");
    }
    if (!LoadPickingShader()) {
        spdlog::warn("ShaderLibrary: Picking shader not loaded - object picking will be disabled");
    }
    if (!LoadPrimitiveShader()) {
        spdlog::warn("ShaderLibrary: Primitive shader not loaded - debug visualization will be disabled");
    }
    if (!LoadDebugLineShader()) {
        spdlog::warn("ShaderLibrary: Debug line shader not loaded - physics debug visualization will be disabled");
    }
    if (!LoadOutlineShader()) {
        spdlog::warn("ShaderLibrary: Outline shader not loaded - object outlines will be disabled");
    }
    if (!LoadSkyboxShader()) {
        spdlog::warn("ShaderLibrary: Skybox shader not loaded - skybox rendering will be disabled");
    }
    if (!LoadHDRComputeShader()) {
        spdlog::warn("ShaderLibrary: HDR compute shader not loaded - auto-exposure will be disabled");
    }
    if (!LoadToneMappingShader()) {
        spdlog::warn("ShaderLibrary: Tone mapping shader not loaded - HDR tone mapping will be disabled");
    }
    if (!LoadEditorResolveShader()) {
        spdlog::warn("ShaderLibrary: Editor resolve shader not loaded - editor viewport gamma correction will be disabled");
    }

    if (!LoadParticleShader())
    {
        spdlog::warn("ShaderLibrary: Particle Shader not loaded - Particle will be disabled");
    }
    if (!LoadHUDShader()) {
        spdlog::warn("ShaderLibrary: HUD shader not loaded - HUD rendering will be disabled");
    }
    if (!LoadTextShader()) {
        spdlog::warn("ShaderLibrary: Text shader not loaded - SDF text rendering will be disabled");
    }
    if (!LoadWorldTextShader()) {
        spdlog::warn("ShaderLibrary: World text shader not loaded - world-space text rendering will be disabled");
    }
    if (!LoadWorldUIShader()) {
        spdlog::warn("ShaderLibrary: World UI shader not loaded - world-space UI rendering will be disabled");
    }

    if (success) {
        spdlog::info("ShaderLibrary: Essential shaders loaded successfully");
    } else {
        spdlog::error("ShaderLibrary: Failed to load critical shaders");
    }

    return success;
}

std::shared_ptr<Shader> ShaderLibrary::Load(
    const std::string& name,
    const std::string& vertPath,
    const std::string& fragPath)
{
    // Check if already loaded
    if (m_Shaders.contains(name)) {
        spdlog::debug("ShaderLibrary: Shader '{}' already loaded", name);
        return m_Shaders[name];
    }

    // Load via ResourceManager
    auto shader = m_ResourceManager->LoadShader(name, vertPath, fragPath);
    if (shader) {
        m_Shaders[name] = shader;
        spdlog::debug("ShaderLibrary: Loaded shader '{}'", name);
    } else {
        spdlog::error("ShaderLibrary: Failed to load shader '{}'", name);
    }

    return shader;
}

std::shared_ptr<Shader> ShaderLibrary::LoadWithGeometry(
    const std::string& name,
    const std::string& vertPath,
    const std::string& fragPath,
    const std::string& geomPath)
{
    // Check if already loaded
    if (m_Shaders.contains(name)) {
        spdlog::debug("ShaderLibrary: Shader '{}' already loaded", name);
        return m_Shaders[name];
    }

    // Load via ResourceManager
    auto shader = m_ResourceManager->LoadShaderWithGeometry(name, vertPath, fragPath, geomPath);
    if (shader) {
        m_Shaders[name] = shader;
        spdlog::debug("ShaderLibrary: Loaded geometry shader '{}'", name);
    }
    else {
        spdlog::error("ShaderLibrary: Failed to load geometry shader '{}'", name);
    }

    return shader;
}

std::shared_ptr<Shader> ShaderLibrary::LoadCompute(
    const std::string& name,
    const std::string& compPath)
{
    // Check if already loaded
    if (m_Shaders.contains(name)) {
        spdlog::debug("ShaderLibrary: Compute shader '{}' already loaded", name);
        return m_Shaders[name];
    }

    // Load via ResourceManager
    auto shader = m_ResourceManager->LoadComputeShader(name, compPath);
    if (shader) {
        m_Shaders[name] = shader;
        spdlog::debug("ShaderLibrary: Loaded compute shader '{}'", name);
    }
    else {
        spdlog::error("ShaderLibrary: Failed to load compute shader '{}'", name);
    }

    return shader;
}

std::shared_ptr<Shader> ShaderLibrary::Get(const std::string& name) const {
    auto it = m_Shaders.find(name);
    return (it != m_Shaders.end()) ? it->second : nullptr;
}

bool ShaderLibrary::Has(const std::string& name) const {
    return m_Shaders.contains(name);
}

void ShaderLibrary::Clear() {
    m_Shaders.clear();
    m_PBRShader.reset();
    m_ShadowShader.reset();
    m_PickingShader.reset();
    m_PrimitiveShader.reset();
    m_PointShadowShader.reset();
    m_OutlineShader.reset();
    m_SkyboxShader.reset();
    m_HDRComputeShader.reset();
    m_ToneMappingShader.reset();
    m_HUDShader.reset();
    spdlog::info("ShaderLibrary: Cleared all shaders");
}

bool ShaderLibrary::LoadPBRShader() {
    m_PBRShader = Load("main_pbr",
        "assets/shaders/main_pbr.vert",
        "assets/shaders/main_pbr.frag");

    if (!m_PBRShader) {
        spdlog::error("ShaderLibrary: Failed to load PBR shader (CRITICAL - required for rendering)");
        return false;
    }

    spdlog::info("ShaderLibrary: PBR shader loaded successfully");
    return true;
}

bool ShaderLibrary::LoadShadowShader() {
    // Load INSTANCED shadow shader (used for directional and spot lights)
    m_ShadowShader = Load("shadow_depth_instanced",
        "assets/shaders/shadow_depth_instanced.vert",
        "assets/shaders/shadow_depth_instanced.frag");

    if (!m_ShadowShader) {
        return false;
    }

    spdlog::info("ShaderLibrary: Shadow shader loaded successfully");
    return true;
}

bool ShaderLibrary::LoadPickingShader() {
    m_PickingShader = Load("picking",
        "assets/shaders/picking.vert",
        "assets/shaders/picking.frag");

    if (!m_PickingShader) {
        return false;
    }

    spdlog::info("ShaderLibrary: Picking shader loaded successfully");
    return true;
}

bool ShaderLibrary::LoadPrimitiveShader() {
    m_PrimitiveShader = Load("primitive",
        "assets/shaders/primitive.vert",
        "assets/shaders/primitive.frag");

    if (!m_PrimitiveShader) {
        return false;
    }

    spdlog::info("ShaderLibrary: Primitive shader loaded successfully");
    return true;
}

bool ShaderLibrary::LoadDebugLineShader() {
    m_DebugLineShader = Load("debug_line",
        "assets/shaders/debug_line.vert",
        "assets/shaders/debug_line.frag");

    if (!m_DebugLineShader) {
        return false;
    }

    spdlog::info("ShaderLibrary: Debug line shader loaded successfully");
    return true;
}

bool ShaderLibrary::LoadPointShadowShader() {
    // Load INSTANCED point shadow shader with geometry shader (for cubemap generation)
    m_PointShadowShader = LoadWithGeometry("point_shadow_instanced",
        "assets/shaders/point_shadow_instanced.vert",
        "assets/shaders/point_shadow_instanced.frag",
        "assets/shaders/point_shadow_instanced.geom");

    if (!m_PointShadowShader) {
        return false;
    }

    spdlog::info("ShaderLibrary: Instanced point shadow shader loaded successfully");
    return true;
}

bool ShaderLibrary::LoadOutlineShader() {
    m_OutlineShader = Load("outline",
        "assets/shaders/outline.vert",
        "assets/shaders/outline.frag");

    if (!m_OutlineShader) {
        return false;
    }

    spdlog::info("ShaderLibrary: Outline shader loaded successfully");
    return true;
}

bool ShaderLibrary::LoadSkyboxShader() {
    m_SkyboxShader = Load("skybox",
        "assets/shaders/skybox.vert",
        "assets/shaders/skybox.frag");

    if (!m_SkyboxShader) {
        return false;
    }

    spdlog::info("ShaderLibrary: Skybox shader loaded successfully");
    return true;
}

bool ShaderLibrary::LoadHDRComputeShader() {
    m_HDRComputeShader = LoadCompute("hdr_luminance",
        "assets/shaders/hdr_luminance.comp");

    if (!m_HDRComputeShader) {
        return false;
    }

    spdlog::info("ShaderLibrary: HDR compute shader loaded successfully");
    return true;
}

bool ShaderLibrary::LoadToneMappingShader() {
    m_ToneMappingShader = Load("tonemap",
        "assets/shaders/tonemap.vert",
        "assets/shaders/tonemap.frag");

    if (!m_ToneMappingShader) {
        return false;
    }

    spdlog::info("ShaderLibrary: Tone mapping shader loaded successfully");
    return true;
}

bool ShaderLibrary::LoadEditorResolveShader() {
    m_EditorResolveShader = Load("editor_resolve",
        "assets/shaders/editor_resolve.vert",
        "assets/shaders/editor_resolve.frag");

    if (!m_EditorResolveShader) {
        return false;
    }

    spdlog::info("ShaderLibrary: Editor resolve shader loaded successfully");
    return true;
}

bool ShaderLibrary::LoadParticleShader()
{
    m_ParticleShader = Load("particle", "assets/shaders/particle.vert", "assets/shaders/particle.frag");
    if (!m_ParticleShader)
    {
        return false;
    }
    spdlog::info("ShaderLibrary: Particle shader loaded successfully");
    return true;
}

bool ShaderLibrary::LoadHUDShader() {
    m_HUDShader = Load("hud",
        "assets/shaders/hud.vert",
        "assets/shaders/hud.frag");

    if (!m_HUDShader) {
        return false;
    }

    spdlog::info("ShaderLibrary: HUD shader loaded successfully");
    return true;
}

bool ShaderLibrary::LoadTextShader() {
    m_TextShader = Load("text_sdf",
        "assets/shaders/text_sdf.vert",
        "assets/shaders/text_sdf.frag");

    if (!m_TextShader) {
        return false;
    }

    spdlog::info("ShaderLibrary: SDF text shader loaded successfully");
    return true;
}

bool ShaderLibrary::LoadWorldTextShader() {
    m_WorldTextShader = Load("text_sdf_world",
        "assets/shaders/text_sdf_world.vert",
        "assets/shaders/text_sdf.frag");

    if (!m_WorldTextShader) {
        return false;
    }

    spdlog::info("ShaderLibrary: World SDF text shader loaded successfully");
    return true;
}

bool ShaderLibrary::LoadWorldUIShader() {
    m_WorldUIShader = Load("world_ui",
        "assets/shaders/world_ui.vert",
        "assets/shaders/world_ui.frag");

    if (!m_WorldUIShader) {
        return false;
    }

    spdlog::info("ShaderLibrary: World UI shader loaded successfully");
    return true;
}
