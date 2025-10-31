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
#include <Resources/MaterialInstance.h>
#include <Resources/Material.h>
#include <Resources/Texture.h>
#include <spdlog/spdlog.h>

void MaterialOverridesSystem::Init() {
    spdlog::info("MaterialOverridesSystem: Initializing");

    // MaterialInstanceManager is accessed via Engine::GetRenderSystem()
    // ResourceSystem is a singleton
    m_ResourceSystem = &ResourceSystem::Instance();

    m_EntitiesWithInstances.clear();
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
        const bool hasInstance = m_EntitiesWithInstances.find(entityUID) != m_EntitiesWithInstances.end();

        // Case 1: Has overrides but no instance -> Create instance
        if (hasOverrides && !hasInstance) {
            // Get base material from cache (ComponentInitializer should have loaded it)
            auto baseMaterial = renderSystem.GetEntityMaterial(entityUID);
            if (!baseMaterial) {
                spdlog::warn("MaterialOverridesSystem: Entity {} has overrides but no base material", entityUID);
                continue;
            }

            // Create MaterialInstance via RenderSystem
            auto instance = renderSystem.GetMaterialInstance(entityUID, baseMaterial);
            if (instance) {
                ApplyOverrides(instance, overrides);
                m_EntitiesWithInstances.insert(entityUID);
                spdlog::debug("MaterialOverridesSystem: Created MaterialInstance for entity {}", entityUID);
            }
        }
        // Case 2: Has overrides and has instance -> Update instance (if changed)
        else if (hasOverrides && hasInstance) {
            auto instance = renderSystem.GetExistingMaterialInstance(entityUID);
            if (instance) {
                // TODO: Optimize by tracking component changes (dirty flag)
                // For now, reapply all overrides every frame
                ApplyOverrides(instance, overrides);
            }
        }
        // Case 3: No overrides but has instance -> Destroy instance
        else if (!hasOverrides && hasInstance) {
            renderSystem.DestroyMaterialInstance(entityUID);
            m_EntitiesWithInstances.erase(entityUID);
            spdlog::debug("MaterialOverridesSystem: Destroyed MaterialInstance for entity {}", entityUID);
        }
        // Case 4: No overrides and no instance -> Nothing to do
    }

    // Cleanup tracking for removed entities
    // (This prevents memory leaks if entities are destroyed without component removal)
    std::vector<uint64_t> toRemove;
    for (uint64_t entityUID : m_EntitiesWithInstances) {
        if (!renderSystem.HasMaterialInstance(entityUID)) {
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

std::shared_ptr<MaterialInstance> MaterialOverridesSystem::CreateMaterialInstance(
    uint64_t entityUID,
    std::shared_ptr<Material> baseMaterial,
    const MaterialOverridesComponent& overrides)
{
    auto& renderSystem = Engine::GetRenderSystem();
    auto instance = renderSystem.GetMaterialInstance(entityUID, baseMaterial);

    if (instance) {
        ApplyOverrides(instance, overrides);
    }

    return instance;
}

void MaterialOverridesSystem::ApplyOverrides(
    std::shared_ptr<MaterialInstance> instance,
    const MaterialOverridesComponent& overrides)
{
    if (!instance) return;

    // Apply float overrides
    for (const auto& [name, value] : overrides.floatOverrides) {
        instance->SetFloat(name, value);
    }

    // Apply vec3 overrides
    for (const auto& [name, value] : overrides.vec3Overrides) {
        instance->SetVec3(name, value);
    }

    // Apply vec4 overrides
    for (const auto& [name, value] : overrides.vec4Overrides) {
        instance->SetVec4(name, value);
    }

    // Apply mat4 overrides
    for (const auto& [name, value] : overrides.mat4Overrides) {
        instance->SetMat4(name, value);
    }

    // Apply texture overrides
    for (const auto& [name, textureGuid] : overrides.textureOverrides) {
        auto texture = LoadTexture(textureGuid);
        if (texture) {
            instance->SetTexture(name, texture);
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
