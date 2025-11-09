/******************************************************************************/
/*!
\file   MaterialInstanceManager.h
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2025/10/20
\brief  Manager for material instances lifecycle

        Manages per-object material instances, implementing Unity's
        sharedMaterial vs material semantics.

        Framework-agnostic: Uses uint64_t IDs instead of ECS entities.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <Resources/MaterialInstance.h>
#include <memory>
#include <unordered_map>
#include <cstdint>

/**
 * @brief Manages material instances for objects
 *
 * Framework-agnostic material instance manager using generic uint64_t IDs.
 * The engine layer is responsible for converting ECS entities to IDs.
 *
 * Provides Unity-style material instance management:
 * - GetSharedMaterial(): Returns the shared asset
 * - GetMaterialInstance(): Returns or creates a per-object instance
 * - SetSharedMaterial(): Changes the base material (destroys instance)
 */
class MaterialInstanceManager {
public:
    using ObjectID = uint64_t;  ///< Generic object identifier (entity ID, handle, etc.)

    MaterialInstanceManager() = default;
    ~MaterialInstanceManager() = default;

    /**
     * @brief Get the shared material for an object (no instancing)
     * @param objectID Unique identifier for the object
     * @param baseMaterial The base material asset
     * @return The shared material (can be nullptr)
     */
    std::shared_ptr<Material> GetSharedMaterial(std::shared_ptr<Material> baseMaterial) const;

    /**
     * @brief Get or create a material instance for an object
     *
     * First call creates an instance, subsequent calls return the existing instance.
     * This implements Unity's Renderer.material behavior (copy-on-write).
     *
     * @param objectID Unique identifier for the object
     * @param baseMaterial The base material to instance from
     * @return Material instance (never null if baseMaterial is valid)
     */
    std::shared_ptr<MaterialInstance> GetMaterialInstance(ObjectID objectID,
                                                           std::shared_ptr<Material> baseMaterial);

    /**
     * @brief Check if an object has a material instance
     */
    bool HasInstance(ObjectID objectID) const;

    /**
     * @brief Get existing instance (returns nullptr if no instance exists)
     */
    std::shared_ptr<MaterialInstance> GetExistingInstance(ObjectID objectID) const;

    /**
     * @brief Set the shared material for an object (destroys instance if exists)
     *
     * This resets the object to use the shared material, clearing any instance.
     *
     * @param objectID Unique identifier for the object
     * @param baseMaterial The new shared material
     */
    void SetSharedMaterial(ObjectID objectID, std::shared_ptr<Material> baseMaterial);

    /**
     * @brief Destroy the material instance for an object
     *
     * After this call, the object will use the shared material again.
     *
     * @param objectID Unique identifier for the object
     */
    void DestroyInstance(ObjectID objectID);

    /**
     * @brief Get total number of active instances
     */
    size_t GetInstanceCount() const { return m_Instances.size(); }

    /**
     * @brief Clear all instances (useful for scene unload)
     */
    void ClearAllInstances();

private:
    /// Map: object ID -> material instance
    std::unordered_map<ObjectID, std::shared_ptr<MaterialInstance>> m_Instances;
};
