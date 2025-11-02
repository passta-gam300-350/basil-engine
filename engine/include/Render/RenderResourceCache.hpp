#ifndef ENGINE_RENDER_RESOURCE_CACHE_HPP
#define ENGINE_RENDER_RESOURCE_CACHE_HPP
#pragma once

#include <Resources/Mesh.h>
#include <Resources/Material.h>
#include <rsc-core/rp.hpp>
#include <memory>
#include <unordered_map>

/**
 * @brief Manages runtime resource caching for the render system
 *
 * Responsibilities:
 * - Cache entity-to-resource mappings (entity UID -> Mesh/Material)
 * - Cache editor-provided resources (GUID -> Mesh/Material)
 * - Provide resource lookup methods
 * - Support cache invalidation and cleanup
 *
 * Cache Types:
 * 1. Entity Cache: Maps entity UID to initialized resources (for rendering)
 * 2. Editor Cache: Maps GUIDs to editor-provided in-memory resources
 *
 * Lifetime: Owned by RenderSystem
 */
class RenderResourceCache {
public:
    RenderResourceCache() = default;
    ~RenderResourceCache() = default;

    // Delete copy/move to prevent accidental duplication
    RenderResourceCache(const RenderResourceCache&) = delete;
    RenderResourceCache& operator=(const RenderResourceCache&) = delete;
    RenderResourceCache(RenderResourceCache&&) = delete;
    RenderResourceCache& operator=(RenderResourceCache&&) = delete;

    // ========== Entity Resource Cache (entity UID -> resource) ==========

    /**
     * @brief Get cached mesh for an entity
     * @param entityUID Entity unique identifier
     * @return Mesh pointer, or nullptr if not cached
     */
    std::shared_ptr<Mesh> GetEntityMesh(uint64_t entityUID) const;

    /**
     * @brief Get cached material for an entity
     * @param entityUID Entity unique identifier
     * @return Material pointer, or nullptr if not cached
     */
    std::shared_ptr<Material> GetEntityMaterial(uint64_t entityUID) const;

    /**
     * @brief Cache a mesh for an entity
     * @param entityUID Entity unique identifier
     * @param mesh Mesh to cache
     */
    void SetEntityMesh(uint64_t entityUID, std::shared_ptr<Mesh> mesh);

    /**
     * @brief Cache a material for an entity
     * @param entityUID Entity unique identifier
     * @param material Material to cache
     */
    void SetEntityMaterial(uint64_t entityUID, std::shared_ptr<Material> material);

    /**
     * @brief Check if entity has cached mesh and material
     * @param entityUID Entity unique identifier
     * @return true if both mesh and material are cached
     */
    bool HasEntityResources(uint64_t entityUID) const;

    /**
     * @brief Clear all resources for a specific entity
     * @param entityUID Entity unique identifier
     */
    void ClearEntityResources(uint64_t entityUID);

    /**
     * @brief Clear all entity resource caches
     */
    void ClearAllEntityCaches();

    // ========== Editor Resource Cache (GUID -> resource) ==========

    /**
     * @brief Get editor-registered mesh by GUID
     * @param guid Resource GUID
     * @return Mesh pointer, or nullptr if not registered
     */
    std::shared_ptr<Mesh> GetEditorMesh(const rp::Guid& guid) const;

    /**
     * @brief Get editor-registered material by GUID
     * @param guid Resource GUID
     * @return Material pointer, or nullptr if not registered
     */
    std::shared_ptr<Material> GetEditorMaterial(const rp::Guid& guid) const;

    /**
     * @brief Register an editor mesh (in-memory asset)
     * @param guid Resource GUID
     * @param mesh Mesh to register
     */
    void RegisterEditorMesh(const rp::Guid& guid, std::shared_ptr<Mesh> mesh);

    /**
     * @brief Register an editor material (in-memory asset)
     * @param guid Resource GUID
     * @param material Material to register
     */
    void RegisterEditorMaterial(const rp::Guid& guid, std::shared_ptr<Material> material);

    /**
     * @brief Clear all editor resource caches
     */
    void ClearAllEditorCaches();

    /**
     * @brief Clear all caches (entity + editor)
     */
    void ClearAll();

private:
    // Entity-to-resource mappings (keyed by entity UID)
    // These are populated at component initialization time
    std::unordered_map<uint64_t, std::shared_ptr<Mesh>> m_EntityMeshCache;
    std::unordered_map<uint64_t, std::shared_ptr<Material>> m_EntityMaterialCache;

    // Editor-provided resource mappings (keyed by GUID)
    // These are populated by editor before entity initialization
    std::unordered_map<rp::Guid, std::shared_ptr<Mesh>> m_EditorMeshCache;
    std::unordered_map<rp::Guid, std::shared_ptr<Material>> m_EditorMaterialCache;
};

#endif // ENGINE_RENDER_RESOURCE_CACHE_HPP
