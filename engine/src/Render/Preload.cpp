/******************************************************************************/
/*!
\file   Preload.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/03/28
\brief  Non-blocking scene preloading system implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#include "Render/Preload.hpp"
#include "Manager/ResourceSystem.hpp"
#include <rsc-core/rp.hpp>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <unordered_set>
#include <Render/Render.h>

namespace preload {

PreloadManager& PreloadManager::Instance() {
    static PreloadManager instance;
    return instance;
}

bool PreloadManager::BeginPreload(const std::string& scenePath,
                                   ProgressCallback onProgress,
                                   ErrorCallback onError) {
    if (m_State.isActive) {
        spdlog::warn("PreloadManager: Cannot start new preload while one is active");
        return false;
    }
    
    if (!std::filesystem::exists(scenePath)) {
        spdlog::error("PreloadManager: Scene file not found: {}", scenePath);
        return false;
    }
    
    m_State = PreloadState{};
    m_PendingScene.reset();
    
    try {
        YAML::Node sceneYaml = YAML::LoadFile(scenePath);
        m_PendingScene = PendingSceneData{};
        m_PendingScene->scenePath = scenePath;
        m_PendingScene->sceneYamlNode = sceneYaml;
        
        if (sceneYaml["scene"]) {
            if (sceneYaml["scene"]["name"]) {
                m_PendingScene->sceneName = sceneYaml["scene"]["name"].as<std::string>();
            }
            if (sceneYaml["scene"]["guid"]) {
                m_PendingScene->sceneGuidHex = sceneYaml["scene"]["guid"].as<std::string>();
            }
        }
        
        ExtractResourcesFromYaml(sceneYaml, m_PendingScene->resources);
        m_PendingScene->yamlParsed = true;
        
        m_State.pendingResources = m_PendingScene->resources;
        m_State.progress.totalResources = m_State.pendingResources.size();
        m_State.progress.loadedResources = 0;
        m_State.progress.failedResources = 0;
        m_State.progress.isComplete = false;
        m_State.progress.hasErrors = false;
        
        m_State.onProgress = std::move(onProgress);
        m_State.onError = std::move(onError);
        m_State.isActive = true;
        m_State.isPaused = false;
        
        spdlog::info("PreloadManager: Started preload for scene '{}' ({}, {} resources queued)",
                     m_PendingScene->sceneName, scenePath, m_State.progress.totalResources);
        
        if (m_State.onProgress) {
            m_State.onProgress(m_State.progress);
        }
        
        return true;
    }
    catch (const YAML::Exception& e) {
        spdlog::error("PreloadManager: Failed to parse scene YAML: {}", e.what());
        m_PendingScene.reset();
        return false;
    }
}

bool PreloadManager::ProcessBatch() {
    if (!m_State.isActive || m_State.isPaused) {
        return m_State.isActive;
    }
    
    if (m_State.pendingResources.empty()) {
        m_State.progress.isComplete = true;
        m_State.isActive = false;
        
        spdlog::info("PreloadManager: Preload complete for scene '{}' ({} loaded, {} failed)",
                     m_PendingScene.has_value() ? m_PendingScene->sceneName : "unknown",
                     m_State.progress.loadedResources,
                     m_State.progress.failedResources);
        
        if (m_State.onProgress) {
            m_State.onProgress(m_State.progress);
        }
        
        return false;
    }
    
    QueuedResource resource = m_State.pendingResources.front();
    m_State.pendingResources.erase(m_State.pendingResources.begin());
    
    m_State.progress.currentResourceName = resource.debugName;
    m_State.progress.currentResourceGuid = resource.guidHex;
    
    bool success = LoadSingleResource(resource);
    
    if (success) {
        m_State.progress.loadedResources++;
    } else {
        m_State.progress.failedResources++;
        m_State.progress.hasErrors = true;
        
        if (m_State.onError) {
            m_State.onError(resource.guidHex, "Failed to load resource");
        }
    }
    
    if (m_State.onProgress) {
        m_State.onProgress(m_State.progress);
    }
    
    return m_State.isActive;
}

void PreloadManager::Cancel() {
    if (m_State.isActive) {
        spdlog::info("PreloadManager: Cancelling preload for scene '{}'",
                     m_PendingScene.has_value() ? m_PendingScene->sceneName : "unknown");
    }
    
    m_State = PreloadState{};
    m_PendingScene.reset();
}

void PreloadManager::ExtractResourcesFromYaml(
    const YAML::Node& sceneYaml,
    std::vector<QueuedResource>& outResources) {
    
    outResources.clear();
    std::unordered_set<std::string> seenGuids;
    
    if (!sceneYaml["entities"] || !sceneYaml["entities"].IsSequence()) {
        return;
    }
    
    const YAML::Node& entities = sceneYaml["entities"];
    
    for (const auto& entityNode : entities) {
        if (entityNode["MeshRendererComponent"]) {
            const YAML::Node& meshComp = entityNode["MeshRendererComponent"];
            
            if (meshComp["m_MeshGuid"] && meshComp["m_MeshGuid"]["m_guid"]) {
                std::string meshGuidHex = meshComp["m_MeshGuid"]["m_guid"].as<std::string>();
                if (!meshGuidHex.empty() && meshGuidHex != "00000000000000000000000000000000") {
                    if (seenGuids.find(meshGuidHex) == seenGuids.end()) {
                        seenGuids.insert(meshGuidHex);
                        outResources.push_back({meshGuidHex, ResourceType::Mesh, "Mesh:" + meshGuidHex.substr(0, 8)});
                    }
                    
                    rp::Guid meshGuid = rp::Guid::to_guid(meshGuidHex);
                    rp::Guid metaGuid = meshGuid;
                    metaGuid.m_low += 1;
                    std::string metaGuidHex = metaGuid.to_hex();
                    if (seenGuids.find(metaGuidHex) == seenGuids.end()) {
                        seenGuids.insert(metaGuidHex);
                        outResources.push_back({metaGuidHex, ResourceType::MeshMeta, "MeshMeta:" + metaGuidHex.substr(0, 8)});
                    }
                }
            }
            
            if (meshComp["m_MaterialGuid"] && meshComp["m_MaterialGuid"].IsSequence()) {
                for (const auto& matSlot : meshComp["m_MaterialGuid"]) {
                    if (matSlot.second && matSlot.second["m_guid"]) {
                        std::string matGuidHex = matSlot.second["m_guid"].as<std::string>();
                        if (!matGuidHex.empty() && matGuidHex != "00000000000000000000000000000000") {
                            if (seenGuids.find(matGuidHex) == seenGuids.end()) {
                                seenGuids.insert(matGuidHex);
                                outResources.push_back({matGuidHex, ResourceType::Material, "Material:" + matGuidHex.substr(0, 8)});
                            }
                        }
                    }
                }
            }
            else if (meshComp["m_MaterialGuid"] && meshComp["m_MaterialGuid"]["m_guid"]) {
                std::string matGuidHex = meshComp["m_MaterialGuid"]["m_guid"].as<std::string>();
                if (!matGuidHex.empty() && matGuidHex != "00000000000000000000000000000000") {
                    if (seenGuids.find(matGuidHex) == seenGuids.end()) {
                        seenGuids.insert(matGuidHex);
                        outResources.push_back({matGuidHex, ResourceType::Material, "Material:" + matGuidHex.substr(0, 8)});
                    }
                }
            }
        }
        
        if (entityNode["HUDComponent"]) {
            const YAML::Node& hudComp = entityNode["HUDComponent"];
            if (hudComp["m_TextureGuid"] && hudComp["m_TextureGuid"]["m_guid"]) {
                std::string texGuidHex = hudComp["m_TextureGuid"]["m_guid"].as<std::string>();
                if (!texGuidHex.empty() && texGuidHex != "00000000000000000000000000000000") {
                    if (seenGuids.find(texGuidHex) == seenGuids.end()) {
                        seenGuids.insert(texGuidHex);
                        outResources.push_back({texGuidHex, ResourceType::Texture, "Texture:" + texGuidHex.substr(0, 8)});
                    }
                }
            }
        }
        
        if (entityNode["TextComponent"]) {
            const YAML::Node& textComp = entityNode["TextComponent"];
            if (textComp["m_FontGuid"] && textComp["m_FontGuid"]["m_guid"]) {
                std::string fontGuidHex = textComp["m_FontGuid"]["m_guid"].as<std::string>();
                if (!fontGuidHex.empty() && fontGuidHex != "00000000000000000000000000000000") {
                    if (seenGuids.find(fontGuidHex) == seenGuids.end()) {
                        seenGuids.insert(fontGuidHex);
                        outResources.push_back({fontGuidHex, ResourceType::FontAtlas, "FontAtlas:" + fontGuidHex.substr(0, 8)});
                    }
                }
            }
        }
        
        if (entityNode["TextMeshComponent"]) {
            const YAML::Node& textMeshComp = entityNode["TextMeshComponent"];
            if (textMeshComp["m_FontGuid"] && textMeshComp["m_FontGuid"]["m_guid"]) {
                std::string fontGuidHex = textMeshComp["m_FontGuid"]["m_guid"].as<std::string>();
                if (!fontGuidHex.empty() && fontGuidHex != "00000000000000000000000000000000") {
                    if (seenGuids.find(fontGuidHex) == seenGuids.end()) {
                        seenGuids.insert(fontGuidHex);
                        outResources.push_back({fontGuidHex, ResourceType::FontAtlas, "FontAtlas:" + fontGuidHex.substr(0, 8)});
                    }
                }
            }
        }
        
        if (entityNode["WorldUIComponent"]) {
            const YAML::Node& worldUIComp = entityNode["WorldUIComponent"];
            if (worldUIComp["m_TextureGuid"] && worldUIComp["m_TextureGuid"]["m_guid"]) {
                std::string texGuidHex = worldUIComp["m_TextureGuid"]["m_guid"].as<std::string>();
                if (!texGuidHex.empty() && texGuidHex != "00000000000000000000000000000000") {
                    if (seenGuids.find(texGuidHex) == seenGuids.end()) {
                        seenGuids.insert(texGuidHex);
                        outResources.push_back({texGuidHex, ResourceType::Texture, "Texture:" + texGuidHex.substr(0, 8)});
                    }
                }
            }
        }
    }
    
    if (sceneYaml["render_settings"] && sceneYaml["render_settings"]["skybox"]) {
        const YAML::Node& skybox = sceneYaml["render_settings"]["skybox"];
        if (skybox["face_textures"] && skybox["face_textures"].IsSequence()) {
            for (size_t i = 0; i < skybox["face_textures"].size() && i < 6; ++i) {
                std::string texGuidHex = skybox["face_textures"][i].as<std::string>();
                if (!texGuidHex.empty() && texGuidHex != "00000000000000000000000000000000") {
                    if (seenGuids.find(texGuidHex) == seenGuids.end()) {
                        seenGuids.insert(texGuidHex);
                        outResources.push_back({texGuidHex, ResourceType::Texture, "SkyboxFace:" + texGuidHex.substr(0, 8)});
                    }
                }
            }
        }
    }
    
    spdlog::debug("PreloadManager: Extracted {} unique resources from scene YAML", outResources.size());
}

bool PreloadManager::LoadSingleResource(const QueuedResource& resource) {
    rp::Guid guid = rp::Guid::to_guid(resource.guidHex);
    
    if (guid == rp::null_guid) {
        spdlog::error("PreloadManager: Invalid GUID format: {}", resource.guidHex);
        return false;
    }
    
    auto& rs = ResourceSystem::Instance();
    auto it = rs.m_FileEntries.find(guid);
    if (it == rs.m_FileEntries.end()) {
        spdlog::debug("PreloadManager: Resource GUID not in manifest (may be primitive): {}", resource.guidHex.substr(0, 16));
        return true;
    }
    
    auto& registry = ResourceRegistry::Instance();
    
    try {
        switch (resource.type) {
            case ResourceType::Texture: {
                auto* ptr = registry.Get<std::shared_ptr<Texture>>(guid);
                if (ptr && *ptr) {
                    spdlog::debug("PreloadManager: Loaded texture {} (GPU ID: {})", 
                                  resource.guidHex.substr(0, 8), (*ptr)->id);
                    return true;
                }
                if (registry.Find<std::shared_ptr<Texture>>(guid)) {
                    return true;
                }
                spdlog::error("PreloadManager: Failed to load texture {}", resource.guidHex.substr(0, 8));
                return false;
            }
            
            case ResourceType::Material: {
                auto* ptr = registry.Get<std::shared_ptr<Material>>(guid);
                if (ptr && *ptr) {
                    spdlog::debug("PreloadManager: Loaded material {}", resource.guidHex.substr(0, 8));
                    return true;
                }
                if (registry.Find<std::shared_ptr<Material>>(guid)) {
                    return true;
                }
                spdlog::error("PreloadManager: Failed to load material {}", resource.guidHex.substr(0, 8));
                return false;
            }
            
            case ResourceType::Mesh: {
                using Meshes = std::vector<std::pair<std::string, std::shared_ptr<Mesh>>>;
                auto* ptr = registry.Get<Meshes>(guid);
                if (ptr && !ptr->empty()) {
                    spdlog::debug("PreloadManager: Loaded mesh {} ({} submeshes)", 
                                  resource.guidHex.substr(0, 8), ptr->size());
                    return true;
                }
                if (registry.Find<Meshes>(guid)) {
                    return true;
                }
                spdlog::error("PreloadManager: Failed to load mesh {}", resource.guidHex.substr(0, 8));
                return false;
            }
            default:
                spdlog::warn("PreloadManager: Unknown resource type for GUID {}", resource.guidHex.substr(0, 8));
                return true;
        }
    }
    catch (const std::exception& e) {
        spdlog::error("PreloadManager: Exception loading resource {}: {}", 
                      resource.guidHex.substr(0, 8), e.what());
        return false;
    }
}

} // namespace preload