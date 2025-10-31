#include "Render/ComponentInitializer.hpp"
#include "Render/RenderResourceCache.hpp"
#include "Render/ShaderLibrary.hpp"
#include "Render/PrimitiveManager.hpp"
#include "Render/Render.h"
#include "Manager/ResourceSystem.hpp"
#include "Engine.hpp"
#include <Resources/Material.h>
#include <spdlog/spdlog.h>

ComponentInitializer::ComponentInitializer(
    RenderResourceCache& cache,
    ShaderLibrary& shaderLibrary,
    PrimitiveManager& primitiveManager)
    : m_ResourceCache(cache)
    , m_ShaderLibrary(shaderLibrary)
    , m_PrimitiveManager(primitiveManager)
{
}

void ComponentInitializer::Initialize(entt::registry& registry, entt::entity entity) {
    // Get the MeshRendererComponent
    auto* meshComp = registry.try_get<MeshRendererComponent>(entity);
    if (!meshComp) return;

    // Convert entt::entity to entity ID for caching
    const uint64_t entityUID = static_cast<uint64_t>(ecs::world::detail::entity_id_cast(entity));

    // Skip if already initialized
    if (m_ResourceCache.HasEntityResources(entityUID)) {
        return;
    }

    spdlog::debug("ComponentInitializer: Initializing MeshRenderer for entity {}", entityUID);

    // === MESH INITIALIZATION ===
    if (!m_ResourceCache.GetEntityMesh(entityUID)) {
        std::shared_ptr<Mesh> meshResource;

        // Check if GUID-based resource exists
        meshResource = m_ResourceCache.GetEditorMesh(meshComp->m_MeshGuid);
        if (!meshResource && meshComp->isPrimitive) {
            // Initialize primitive mesh using PrimitiveManager
            switch (meshComp->m_PrimitiveType) {
            case MeshRendererComponent::PrimitiveType::CUBE:
                meshResource = m_PrimitiveManager.GetSharedCubeMesh();
                break;
            case MeshRendererComponent::PrimitiveType::PLANE:
                meshResource = m_PrimitiveManager.GetSharedPlaneMesh();
                break;
            default:
                spdlog::warn("ComponentInitializer: Invalid primitive type for entity {}", entityUID);
                return;
            }
        }
        else if (!meshResource) {
            // Try file-based registry
            auto* meshPtr = ResourceRegistry::Instance().Get<std::shared_ptr<Mesh>>(meshComp->m_MeshGuid);
            if (meshPtr) {
                meshResource = *meshPtr;
            }
        }

        if (meshResource) {
            m_ResourceCache.SetEntityMesh(entityUID, meshResource);
        } else {
            spdlog::warn("ComponentInitializer: Failed to initialize mesh for entity {}", entityUID);
            return;
        }
    }

    // === MATERIAL INITIALIZATION ===
    if (!m_ResourceCache.GetEntityMaterial(entityUID)) {
        std::shared_ptr<Material> materialResource;

        // Check if GUID-based material exists
        materialResource = m_ResourceCache.GetEditorMaterial(meshComp->m_MaterialGuid);
        if (!materialResource && !meshComp->hasAttachedMaterial) {
            // Create default material using ShaderLibrary
            auto pbrShader = m_ShaderLibrary.GetPBRShader();
            if (!pbrShader) {
                spdlog::error("ComponentInitializer: PBR shader not available");
                return;
            }

            // Create default PBR material (no per-entity customization here)
            // Customization is handled by MaterialOverridesSystem
            materialResource = std::make_shared<Material>(pbrShader, "DefaultMaterial_Entity_" + std::to_string(entityUID));
        }
        else if (!materialResource) {
            // Try file-based registry
            auto* materialPtr = ResourceRegistry::Instance().Get<std::shared_ptr<Material>>(meshComp->m_MaterialGuid);
            if (materialPtr) {
                materialResource = *materialPtr;
            }
        }

        if (materialResource) {
            m_ResourceCache.SetEntityMaterial(entityUID, materialResource);
        } else {
            spdlog::warn("ComponentInitializer: Failed to initialize material for entity {}", entityUID);
        }
    }
}

void ComponentInitializer::InitializeAll(ecs::world& world) {
    entt::registry& registry = world.impl.get_registry();

    // Initialize all existing entities with MeshRendererComponent
    auto view = registry.view<MeshRendererComponent>();
    int count = 0;
    for (auto entity : view) {
        Initialize(registry, entity);
        count++;
    }

    spdlog::info("ComponentInitializer: Initialized {} existing MeshRenderer entities", count);
}

void ComponentInitializer::SetupObservers(ecs::world& world) {
    entt::registry& registry = world.impl.get_registry();

    // Disconnect any existing observers first (in case this is called multiple times)
    registry.on_construct<MeshRendererComponent>().disconnect();
    registry.on_destroy<MeshRendererComponent>().disconnect();

    // Set up observer for when MeshRendererComponent is added to an entity
    // EnTT requires either static functions or member function pointers with instance
    // We use the instance + member function pointer overload
    registry.on_construct<MeshRendererComponent>().connect<&ComponentInitializer::Initialize>(this);

    // Set up observer for when MeshRendererComponent is destroyed
    registry.on_destroy<MeshRendererComponent>().connect<&ComponentInitializer::OnMeshRendererDestroyed>(this);

    spdlog::info("ComponentInitializer: Component observers registered (construct + destroy)");
}

void ComponentInitializer::OnMeshRendererDestroyed(entt::registry& registry, entt::entity entity) {
    // Convert entt::entity to entity ID for cache lookup
    const uint64_t entityUID = static_cast<uint64_t>(ecs::world::detail::entity_id_cast(entity));

    spdlog::debug("ComponentInitializer: Cleaning up resources for destroyed entity {}", entityUID);

    // Clean up cached mesh and material resources
    m_ResourceCache.ClearEntityResources(entityUID);

    // Clean up material instances and property blocks via RenderSystem
    auto& renderSystem = Engine::GetRenderSystem();

    // Destroy material instance if it exists
    if (renderSystem.HasMaterialInstance(entityUID)) {
        renderSystem.DestroyMaterialInstance(entityUID);
    }

    // Destroy property block if it exists
    if (renderSystem.HasPropertyBlock(entityUID)) {
        renderSystem.DestroyPropertyBlock(entityUID);
    }

    spdlog::debug("ComponentInitializer: Successfully cleaned up all resources for entity {}", entityUID);
}
