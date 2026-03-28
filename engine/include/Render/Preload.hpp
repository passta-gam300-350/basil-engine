/******************************************************************************/
/*!
\file   Preload.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/03/28
\brief  Non-blocking scene preloading system for render resources

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef ENGINE_PRELOAD_HPP
#define ENGINE_PRELOAD_HPP

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>
#include <yaml-cpp/yaml.h>
#include <rsc-core/rp.hpp>

struct SceneRenderSettings;

namespace preload {

/**
 * @brief Progress tracking for async scene preloading
 */
struct Progress {
    size_t totalResources{0};
    size_t loadedResources{0};
    size_t failedResources{0};
    std::string currentResourceName;
    std::string currentResourceGuid;
    bool isComplete{false};
    bool hasErrors{false};
    
    float GetPercent() const {
        if (totalResources == 0) return 0.0f;
        return static_cast<float>(loadedResources) / static_cast<float>(totalResources) * 100.0f;
    }
};

/**
 * @brief Callback types for preload operations
 */
using ProgressCallback = std::function<void(const Progress&)>;
using ErrorCallback = std::function<void(const std::string& guidHex, const std::string& error)>;

/**
 * @brief Resource types that can be preloaded
 */
enum class ResourceType : uint8_t {
    Texture,
    Material,
    Mesh,
    MeshMeta,
    FontAtlas,
    COUNT
};

/**
 * @brief A queued resource for preloading
 */
struct QueuedResource {
    std::string guidHex;
    ResourceType type;
    std::string debugName;
};

/**
 * @brief Internal state for incremental preloading
 */
struct PreloadState {
    std::vector<QueuedResource> pendingResources;
    Progress progress;
    ProgressCallback onProgress;
    ErrorCallback onError;
    bool isActive{false};
    bool isPaused{false};
};

/**
 * @brief State for a scene being preloaded (kept separate from active world)
 */
struct PendingSceneData {
    std::string scenePath;
    std::string sceneName;
    std::string sceneGuidHex;
    std::vector<QueuedResource> resources;
    YAML::Node sceneYamlNode;
    bool yamlParsed{false};
};

/**
 * @brief Preload manager for non-blocking resource loading
 * 
 * This system enables loading scene resources incrementally across multiple frames
 * while keeping UI responsive. Resources are loaded into ResourceRegistry (shared GPU memory),
 * but scene entities are NOT created in the active world until activation.
 * 
 * Usage Flow:
 * 1. BeginPreloadScene(path, callback) - Parse scene YAML, queue resources
 * 2. ProcessPreloadBatch() - Load one resource per frame (called by CoreUpdate)
 * 3. On completion, caller activates scene into Engine::GetWorld()
 */
class PreloadManager {
public:
    static PreloadManager& Instance();
    
    /**
     * @brief Begin preloading a scene (non-blocking)
     * 
     * Parses scene YAML file into temporary storage, extracts resource GUIDs,
     * and queues them for incremental loading. Does NOT create entities in
     * the active world yet.
     * 
     * @param scenePath Path to the .yaml scene file
     * @param onProgress Optional callback for progress updates
     * @param onError Optional callback for load failures
     * @return true if preload started, false if already preloading
     */
    bool BeginPreload(const std::string& scenePath,
                      ProgressCallback onProgress = nullptr,
                      ErrorCallback onError = nullptr);
    
    /**
     * @brief Process next resource in preload queue
     * 
     * Loads a single resource (or small batch) to maintain UI responsiveness.
     * Called automatically by Engine::CoreUpdate() when preload is active.
     * 
     * @return true if preload is still active, false if complete or idle
     */
    bool ProcessBatch();
    
    /**
     * @brief Check if a preload operation is active
     */
    bool IsPreloading() const { return m_State.isActive; }
    
    /**
     * @brief Check if preload has completed successfully
     */
    bool IsComplete() const { return m_State.progress.isComplete; }
    
    /**
     * @brief Get current preload progress
     */
    const Progress& GetProgress() const { return m_State.progress; }
    
    /**
     * @brief Pause/resume preloading (e.g., during scene transitions)
     */
    void SetPaused(bool paused) { m_State.isPaused = paused; }
    
    /**
     * @brief Cancel active preload and clear queued resources
     */
    void Cancel();
    
    /**
     * @brief Get pending scene data for activation after preload
     * 
     * Returns the parsed scene data if preload is complete.
     * Caller is responsible for creating entities in Engine::GetWorld().
     */
    std::optional<PendingSceneData>& GetPendingSceneData() { return m_PendingScene; }
    
    /**
     * @brief Clear pending scene data after activation
     */
    void ClearPendingSceneData() { m_PendingScene.reset(); }
    
    /**
     * @brief Extract resources from a scene YAML node without loading
     * 
     * Parses the YAML to find all resource GUIDs used by render components.
     * This is called internally during BeginPreload().
     * 
     * @param sceneYaml The loaded YAML node
     * @param outResources Vector to populate with queued resources
     */
    static void ExtractResourcesFromYaml(
        const YAML::Node& sceneYaml,
        std::vector<QueuedResource>& outResources);
    
    /**
     * @brief Load a single resource into ResourceRegistry
     * 
     * @param resource The resource to load
     * @return true if loaded successfully, false on error
     */
    static bool LoadSingleResource(const QueuedResource& resource);
    
private:
    PreloadManager() = default;
    ~PreloadManager() = default;
    
    PreloadManager(const PreloadManager&) = delete;
    PreloadManager& operator=(const PreloadManager&) = delete;
    
    void UpdateProgress();
    
    PreloadState m_State;
    std::optional<PendingSceneData> m_PendingScene;
};

} // namespace preload

#endif // ENGINE_PRELOAD_HPP