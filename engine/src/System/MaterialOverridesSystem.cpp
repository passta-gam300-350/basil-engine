/******************************************************************************/
/*!
\file   MaterialOverridesSystem.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2025/10/31
\brief  Implementation of MaterialOverridesSystem

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "System/MaterialOverridesSystem.hpp"
#include "Component/MaterialOverridesComponent.hpp"
#include "Render/Render.h"
#include "Engine.hpp"
#include "Manager/ResourceSystem.hpp"
#include <Resources/MaterialPropertyBlock.h>
#include <Resources/Texture.h>
#include <spdlog/spdlog.h>

void MaterialOverridesSystem::Init() {
    spdlog::info("MaterialOverridesSystem: Initializing");

    // MaterialInstanceManager is accessed via Engine::GetRenderSystem()
    // ResourceSystem is a singleton
    m_ResourceSystem = &ResourceSystem::Instance();

    m_EntitiesWithInstances.clear();

    // Note: We don't set up observers here because MaterialOverridesSystem uses filter_entities()
    // in Update() which automatically handles component addition/removal.
    // Cleanup is handled by:
    // 1. Update() Case 3: Clears property block when overrides are cleared
    // 2. ComponentInitializer::OnMeshRendererDestroyed: Destroys property block when MeshRendererComponent is removed
    // 3. Update() lines 91-101: Periodic cleanup of stale tracking entries

    spdlog::info("MaterialOverridesSystem: Initialization complete");
}

void MaterialOverridesSystem::Update(ecs::world& world, float dt) {
    // Lazy initialization (since system is created each frame)
    if (!m_ResourceSystem) {
        m_ResourceSystem = &ResourceSystem::Instance();
    }

    auto& renderSystem = Engine::GetRenderSystem();

    // Query all entities with both MeshRendererComponent and MaterialOverridesComponent
    auto entities = world.filter_entities<MeshRendererComponent, MaterialOverridesComponent>();

    for (auto entity : entities) {
        const uint64_t entityUID = entity.get_uid();

        auto [meshRenderer, overrides] = entity.get<MeshRendererComponent, MaterialOverridesComponent>();

        const bool hasOverrides = overrides.HasOverrides();
        const bool hasPropertyBlock = m_EntitiesWithInstances.find(entityUID) != m_EntitiesWithInstances.end();

        // Case 1: Has overrides but no property block -> Create property block
        if (hasOverrides && !hasPropertyBlock) {
            // Get or create MaterialPropertyBlock via RenderSystem
            auto propBlock = renderSystem.GetPropertyBlock(entityUID);
            if (propBlock) {
                ApplyOverridesToPropertyBlock(propBlock, overrides);
                m_EntitiesWithInstances.insert(entityUID);
                spdlog::info("MaterialOverridesSystem: Created MaterialPropertyBlock for entity {} with {} properties",
                    entityUID, propBlock->GetPropertyCount());
            }
        }
        // Case 2: Has overrides and has property block -> Update property block
        else if (hasOverrides && hasPropertyBlock) {
            auto propBlock = renderSystem.GetPropertyBlock(entityUID);
            if (propBlock) {
                // Clear and reapply (simple approach, could optimize with dirty flags)
                propBlock->Clear();
                ApplyOverridesToPropertyBlock(propBlock, overrides);
            }
        }
        // Case 3: No overrides but has property block -> Clear property block
        else if (!hasOverrides && hasPropertyBlock) {
            renderSystem.ClearPropertyBlock(entityUID);
            m_EntitiesWithInstances.erase(entityUID);
            spdlog::debug("MaterialOverridesSystem: Cleared MaterialPropertyBlock for entity {}", entityUID);
        }
        // Case 4: No overrides and no property block -> Nothing to do
    }

    // Cleanup tracking for removed entities
    // (This prevents memory leaks if entities are destroyed without component removal)
    std::vector<uint64_t> toRemove;
    for (uint64_t entityUID : m_EntitiesWithInstances) {
        if (!renderSystem.HasPropertyBlock(entityUID)) {
            toRemove.push_back(entityUID);
        }
    }
    for (uint64_t entityUID : toRemove) {
        m_EntitiesWithInstances.erase(entityUID);
    }
}

void MaterialOverridesSystem::Exit() {
    spdlog::info("MaterialOverridesSystem: Shutting down");
    m_EntitiesWithInstances.clear();
    m_ResourceSystem = nullptr;
}

void MaterialOverridesSystem::ApplyOverridesToPropertyBlock(
    std::shared_ptr<MaterialPropertyBlock> propBlock,
    const MaterialOverridesComponent& overrides)
{
    if (!propBlock) return;

    // Apply float overrides
    for (const auto& [name, value] : overrides.floatOverrides) {
        propBlock->SetFloat(name, value);
    }

    // Apply vec3 overrides
    for (const auto& [name, value] : overrides.vec3Overrides) {
        propBlock->SetVec3(name, value);
    }

    // Apply vec4 overrides
    for (const auto& [name, value] : overrides.vec4Overrides) {
        propBlock->SetVec4(name, value);
    }

    // Apply mat4 overrides
    for (const auto& [name, value] : overrides.mat4Overrides) {
        propBlock->SetMat4(name, value);
    }

    // Apply texture overrides
    for (const auto& [name, textureGuid] : overrides.textureOverrides) {
        auto texture = LoadTexture(textureGuid);
        if (texture) {
            propBlock->SetTexture(name, texture);
        } else {
            spdlog::warn("MaterialOverridesSystem: Failed to load texture for property '{}'", name);
        }
    }
}

std::shared_ptr<Texture> MaterialOverridesSystem::LoadTexture(const Resource::Guid& textureGuid) {
    if (!m_ResourceSystem) {
        spdlog::error("MaterialOverridesSystem: ResourceSystem not available");
        return nullptr;
    }

    // Try to get texture from ResourceRegistry
    auto* texturePtr = ResourceRegistry::Instance().Get<std::shared_ptr<Texture>>(textureGuid);
    if (texturePtr) {
        return *texturePtr;
    }

    // TODO: If texture not loaded, trigger async load via ResourceSystem
    // For now, just return nullptr and log warning
    spdlog::warn("MaterialOverridesSystem: Texture {} not found in ResourceRegistry",
                 textureGuid.to_hex());
    return nullptr;
}
