#include "Render/RenderResourceCache.hpp"
#include <spdlog/spdlog.h>

// ========== Entity Resource Cache ==========

std::shared_ptr<Mesh> RenderResourceCache::GetEntityMesh(uint64_t entityUID) const {
    auto it = m_EntityMeshCache.find(entityUID);
    return (it != m_EntityMeshCache.end()) ? it->second : nullptr;
}

std::shared_ptr<Material> RenderResourceCache::GetEntityMaterial(uint64_t entityUID) const {
    auto it = m_EntityMaterialCache.find(entityUID);
    return (it != m_EntityMaterialCache.end()) ? it->second : nullptr;
}

void RenderResourceCache::SetEntityMesh(uint64_t entityUID, std::shared_ptr<Mesh> mesh) {
    m_EntityMeshCache[entityUID] = mesh;
}

void RenderResourceCache::SetEntityMaterial(uint64_t entityUID, std::shared_ptr<Material> material) {
    m_EntityMaterialCache[entityUID] = material;
}

bool RenderResourceCache::HasEntityResources(uint64_t entityUID) const {
    return m_EntityMeshCache.contains(entityUID) && m_EntityMaterialCache.contains(entityUID);
}

void RenderResourceCache::ClearEntityResources(uint64_t entityUID) {
    m_EntityMeshCache.erase(entityUID);
    m_EntityMaterialCache.erase(entityUID);
    spdlog::debug("RenderResourceCache: Cleared resources for entity {}", entityUID);
}

void RenderResourceCache::ClearAllEntityCaches() {
    m_EntityMeshCache.clear();
    m_EntityMaterialCache.clear();
    spdlog::info("RenderResourceCache: Cleared all entity resource caches");
}

// ========== Editor Resource Cache ==========

std::shared_ptr<Mesh> RenderResourceCache::GetEditorMesh(const Resource::Guid& guid) const {
    auto it = m_EditorMeshCache.find(guid);
    return (it != m_EditorMeshCache.end()) ? it->second : nullptr;
}

std::shared_ptr<Material> RenderResourceCache::GetEditorMaterial(const Resource::Guid& guid) const {
    auto it = m_EditorMaterialCache.find(guid);
    return (it != m_EditorMaterialCache.end()) ? it->second : nullptr;
}

void RenderResourceCache::RegisterEditorMesh(const Resource::Guid& guid, std::shared_ptr<Mesh> mesh) {
    m_EditorMeshCache[guid] = mesh;
    spdlog::debug("RenderResourceCache: Registered editor mesh with GUID {}", guid.to_hex());
}

void RenderResourceCache::RegisterEditorMaterial(const Resource::Guid& guid, std::shared_ptr<Material> material) {
    m_EditorMaterialCache[guid] = material;
    spdlog::debug("RenderResourceCache: Registered editor material with GUID {}", guid.to_hex());
}

void RenderResourceCache::ClearAllEditorCaches() {
    m_EditorMeshCache.clear();
    m_EditorMaterialCache.clear();
    spdlog::info("RenderResourceCache: Cleared all editor resource caches");
}

// ========== Combined Operations ==========

void RenderResourceCache::ClearAll() {
    ClearAllEntityCaches();
    ClearAllEditorCaches();
    spdlog::info("RenderResourceCache: Cleared all caches");
}
