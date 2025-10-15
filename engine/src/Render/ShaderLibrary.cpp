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
    if (!LoadPickingShader()) {
        spdlog::warn("ShaderLibrary: Picking shader not loaded - object picking will be disabled");
    }
    if (!LoadPrimitiveShader()) {
        spdlog::warn("ShaderLibrary: Primitive shader not loaded - debug visualization will be disabled");
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
    m_ShadowShader = Load("shadow_depth",
        "assets/shaders/shadow_depth.vert",
        "assets/shaders/shadow_depth.frag");

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
