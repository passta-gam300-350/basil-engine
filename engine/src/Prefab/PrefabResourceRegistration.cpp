#include "Prefab/PrefabData.hpp"
#include "System/PrefabSystem.hpp"
#include "Manager/ResourceSystem.hpp"
#include <memory>
#include <spdlog/spdlog.h>

/**
 * @brief Loader function for PrefabData resources
 *
 * This function is called asynchronously when a prefab is requested but not in cache.
 * It loads the prefab from disk using the file path provided by the ResourceSystem.
 *
 * @param filePath Path to the .prefab file
 * @return Loaded PrefabData structure
 * @throws std::exception if loading fails
 */
PrefabData LoadPrefabResource(const char* filePath)
{
    spdlog::info("Loading prefab from: {}", filePath);

    // Use PrefabSystem to load from file
    PrefabData data = PrefabSystem::LoadPrefabFromFile(filePath);

    if (!data.IsValid())
    {
        spdlog::error("Failed to load prefab: {}", filePath);
        throw std::runtime_error("Failed to load prefab file");
    }

    spdlog::debug("Successfully loaded prefab '{}' (version: {})", data.name, data.version);
    return data;
}

/**
 * @brief Unloader function for PrefabData resources
 *
 * This function is called when a prefab is unloaded from the resource pool.
 * Since PrefabData doesn't hold external resources (textures, meshes), this is mostly a no-op.
 *
 * @param data Reference to the PrefabData being unloaded
 */
void UnloadPrefabResource(PrefabData& data)
{
    spdlog::debug("Unloading prefab '{}'", data.name);
    // No external cleanup needed - PrefabData is self-contained
}

// Register PrefabData as a resource type
// This uses global constructor injection to register during static initialization
REGISTER_RESOURCE_TYPE(PrefabData, LoadPrefabResource, UnloadPrefabResource)
