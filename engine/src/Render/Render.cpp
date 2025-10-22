#include <Core/Window.h>
#include "Render/Render.h"
#include "Render/ShaderLibrary.hpp"
#include "Render/PrimitiveManager.hpp"
#include "Render/RenderResourceCache.hpp"
#include "Render/ComponentInitializer.hpp"
#include "Engine.hpp"  // For Engine::GetRenderSystem()
#include "components/transform.h"
#include "Manager/ResourceSystem.hpp"

#include <Resources/MaterialInstanceManager.h>
#include <Resources/MaterialInstance.h>
#include <Resources/MaterialPropertyBlock.h>

#include "native/loader.h"
#include "native/texture.h"

#include <tinyddsloader.h>
#include "Input/InputManager.h"
#include <Resources/PrimitiveGenerator.h>
#include <Resources/Material.h>
#include <Utility/AABB.h>
#include <spdlog/spdlog.h>
#include "Profiler/profiler.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext.hpp>

// Note: Resource caches moved to RenderResourceCache class

RenderSystem::RenderSystem() {
	// Initialize graphics rendering
	m_SceneRenderer = std::make_unique<SceneRenderer>();
	m_Camera = std::make_unique<Camera>(CameraType::Perspective);
	m_Camera->SetPerspective(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
	m_Camera->SetPosition(glm::vec3(0.0f, 2.0f, 8.0f));
	m_Camera->SetRotation(glm::vec3(-10.0f, 0.0f, 0.0f));

	// Initialize shader library
	m_ShaderLibrary = std::make_unique<ShaderLibrary>(
		m_SceneRenderer->GetResourceManager()
	);

	if (!m_ShaderLibrary->Initialize()) {
		spdlog::error("RenderSystem: Failed to initialize ShaderLibrary");
	}

	// Configure SceneRenderer with loaded shaders
	if (m_ShaderLibrary->GetShadowShader()) {
		m_SceneRenderer->SetShadowDepthShader(m_ShaderLibrary->GetShadowShader());
	}
	if (m_ShaderLibrary->GetPickingShader()) {
		m_SceneRenderer->SetPickingShader(m_ShaderLibrary->GetPickingShader());
	}
	if (m_ShaderLibrary->GetPrimitiveShader()) {
		m_SceneRenderer->SetDebugPrimitiveShader(m_ShaderLibrary->GetPrimitiveShader());
	}

	// Initialize primitive manager
	m_PrimitiveManager = std::make_unique<PrimitiveManager>();
	m_PrimitiveManager->Initialize();

	// Initialize resource cache
	m_ResourceCache = std::make_unique<RenderResourceCache>();

	// Initialize material instance manager
	m_MaterialInstanceManager = std::make_unique<MaterialInstanceManager>();

	// Initialize component initializer (depends on all above subsystems)
	m_ComponentInitializer = std::make_unique<ComponentInitializer>(
		*m_ResourceCache,
		*m_ShaderLibrary,
		*m_PrimitiveManager
	);
}

RenderSystem::~RenderSystem() {
	// Release ComponentInitializer first (it depends on other subsystems)
	m_ComponentInitializer.reset();
	m_MaterialInstanceManager.reset();
	m_ResourceCache.reset();
	m_PrimitiveManager.reset();
	m_ShaderLibrary.reset();
	m_SceneRenderer.reset();
	m_Camera.reset();
}

void RenderSystem::Init() {
	// Constructor already initialized everything, just setup debug visualization
	SetupDebugVisualization();

	spdlog::info("RenderSystem initialized");
	spdlog::info("RenderSystem: Call SetupComponentObservers() and InitializeExistingEntities() after world is created");
}

void RenderSystem::InitializeMeshRenderer(entt::registry& registry, entt::entity entity) {
	if (m_ComponentInitializer) {
		m_ComponentInitializer->Initialize(registry, entity);
	} else {
		spdlog::error("RenderSystem: ComponentInitializer not initialized");
	}
}

void RenderSystem::SetupComponentObservers(ecs::world& world) {
	if (m_ComponentInitializer) {
		m_ComponentInitializer->SetupObservers(world);
	} else {
		spdlog::error("RenderSystem: ComponentInitializer not initialized");
	}
}

void RenderSystem::InitializeExistingEntities(ecs::world& world) {
	if (m_ComponentInitializer) {
		m_ComponentInitializer->InitializeAll(world);
	} else {
		spdlog::error("RenderSystem: ComponentInitializer not initialized");
	}
}

void RenderSystem::Update(ecs::world& world) {
	PF_SYSTEM("GraphicSystem");

	//begin frame
	m_SceneRenderer->ClearFrame();

	auto world_cameras = world.filter_entities<CameraComponent, PositionComponent>();
	auto& frameData = m_SceneRenderer->GetFrameData();

	// Camera management - upload ECS camera data to graphics lib camera
	if (world_cameras) {
		auto [camera, camera_pos] {(*world_cameras.begin()).get<CameraComponent, PositionComponent>()};

		// Update graphics camera from ECS camera entity
		m_Camera->SetPosition(camera_pos.m_WorldPos);

		// Set camera orientation using Front, Up, Right vectors
		glm::mat4 view = glm::lookAt(
			camera_pos.m_WorldPos,
			camera_pos.m_WorldPos + camera.m_Front,  // Look at position + front vector
			camera.m_Up
		);

		// Set the view matrix directly instead of using SetRotation
		frameData.viewMatrix = view;

		if (camera.m_Type == CameraComponent::CameraType::PERSPECTIVE) {
			m_Camera->SetPerspective(camera.m_Fov, camera.m_AspectRatio, camera.m_Near, camera.m_Far);
			frameData.projectionMatrix = m_Camera->GetProjectionMatrix();
		}

		frameData.cameraPosition = camera_pos.m_WorldPos;
	}
	else {
		// Fallback if no camera entity exists
		frameData.viewMatrix = m_Camera->GetViewMatrix();
		frameData.projectionMatrix = m_Camera->GetProjectionMatrix();
		frameData.cameraPosition = m_Camera->GetPosition();
	}

	auto sceneObjects = world.filter_entities<MeshRendererComponent, TransformComponent, VisibilityComponent>();
	auto sceneLights = world.filter_entities<LightComponent, PositionComponent>();

	// Debug: Log entity counts
	int objectCount = 0;
	int lightCount = 0;
	for (auto obj : sceneObjects) { objectCount++; }
	for (auto light : sceneLights) { lightCount++; }

	static int lastObjectCount = -1;
	static int lastLightCount = -1;
	if (objectCount != lastObjectCount || lightCount != lastLightCount) {
		spdlog::info("RenderSystem: Processing {} renderable objects, {} lights", objectCount, lightCount);
		lastObjectCount = objectCount;
		lastLightCount = lightCount;
	}

	for (auto obj : sceneObjects) {
		auto [mesh, transform, visible] {obj.get<MeshRendererComponent, TransformComponent, VisibilityComponent>()};
		const uint64_t entityUID = obj.get_uid();

		// === RESOURCE LOOKUP (READ-ONLY - RESOURCES INITIALIZED AT COMPONENT CREATION) ===

		// Get resource cache
		if (!m_ResourceCache) {
			spdlog::error("RenderSystem: ResourceCache not initialized");
			continue;
		}

		// Look up pre-initialized resources from entity cache
		std::shared_ptr<Mesh> meshResource = m_ResourceCache->GetEntityMesh(entityUID);
		std::shared_ptr<Material> materialResource = m_ResourceCache->GetEntityMaterial(entityUID);

		// If resources not found, entity wasn't initialized properly
		// This can happen if observers weren't set up or InitializeExistingEntities wasn't called
		if (!meshResource || !materialResource) {
			// Fallback: Initialize on-demand (shouldn't happen in production)
			static bool warningShown = false;
			if (!warningShown) {
				spdlog::warn("RenderSystem: Entity {} resources not initialized. Call SetupComponentObservers() and InitializeExistingEntities() after world load.", entityUID);
				warningShown = true;
			}

			// Perform lazy initialization as fallback
			entt::registry& registry = world.impl.get_registry();
			entt::entity enttEntity = ecs::world::detail::entt_entity_cast(obj);
			InitializeMeshRenderer(registry, enttEntity);

			// Retry lookup
			meshResource = m_ResourceCache->GetEntityMesh(entityUID);
			materialResource = m_ResourceCache->GetEntityMaterial(entityUID);

			if (!meshResource || !materialResource) {
				continue; // Skip this entity if initialization failed
			}
		}

		// NOTE: Material properties are set during initialization (InitializeMeshRenderer)
		// DO NOT update materials per-frame as this causes shared materials to overwrite each other
		// If you need per-entity material variation, each entity must have its own Material instance

		// Only render if we have both mesh and material
		if (meshResource && materialResource) {
			RenderableData renderData;
			renderData.mesh = meshResource;
			renderData.material = materialResource;
			renderData.transform = transform.m_trans;
			renderData.visible = visible.m_IsVisible;
			renderData.renderLayer = 1;

			// Use existing entityUID (uint64_t) and cast to uint32_t for objectID
			renderData.objectID = static_cast<uint32_t>(entityUID);

			// Attach property block if it exists and has properties
			auto propBlockIt = m_PropertyBlocks.find(entityUID);
			if (propBlockIt != m_PropertyBlocks.end() && !propBlockIt->second->IsEmpty()) {
				renderData.propertyBlock = propBlockIt->second;
			}

			// Debug: Log entity UID assignment for first few entities
			static int debugCount = 0;
			if (debugCount < 5) {
				spdlog::info("RenderSystem: Entity UID assignment - Entity: {}, UID: {}, static_cast result: {}",
					debugCount, entityUID, static_cast<uint32_t>(entityUID));
				debugCount++;
			}

			// Assert entity ID validity for debugging picking
			assert(renderData.objectID != 0 && "Entity UID should not be zero for picking to work");
			assert(renderData.objectID < 16777215 && "Entity UID exceeds 24-bit limit for picking system"); // 24-bit max for RGB encoding

			// Assert mesh validity for rendering
			assert(meshResource->GetVertexArray() && "Mesh must have valid VAO for rendering");
			assert(meshResource->GetVertexArray()->GetVAOHandle() != 0 && "Mesh VAO must be bound to valid OpenGL handle");
			assert(!meshResource->vertices.empty() && "Mesh must have vertices for rendering");

			m_SceneRenderer->SubmitRenderable(renderData);

			// Submit AABB for debug visualization (using pre-calculated mesh AABB)
			if (visible.m_IsVisible && meshResource->GetAABB().IsValid()) {
				DebugAABB debugAABB(meshResource->GetAABB(), transform.m_trans, glm::vec3(1.0f, 0.0f, 0.0f));
				frameData.debugAABBs.push_back(debugAABB);
			}
		}
	}

	for (auto light : sceneLights) {
		auto [lightComponent, position] {light.get<LightComponent, PositionComponent>()};
		SubmittedLightData lightData;
		lightData.type = lightComponent.m_Type;
		lightData.color = lightComponent.m_Color;
		lightData.direction = lightComponent.m_Direction;
		lightData.enabled = lightComponent.m_IsEnabled;
		lightData.innerCone = lightComponent.m_InnerCone;
		lightData.outerCone = lightComponent.m_OuterCone;
		lightData.intensity = lightComponent.m_Intensity;
		lightData.range = lightComponent.m_Range;
		lightData.position = position.m_WorldPos;
		m_SceneRenderer->SubmitLight(lightData);
	}

	//render frame
	m_SceneRenderer->Render();
}
void RenderSystem::FixedUpdate(ecs::world& w) {
	Update(w);
}
void RenderSystem::Exit() {
	// Clear all entity-specific caches before shutdown
	ClearAllEntityCaches();

	// Clear all material instances
	if (m_MaterialInstanceManager) {
		m_MaterialInstanceManager->ClearAllInstances();
	}

	// Clear all property blocks
	m_PropertyBlocks.clear();

	// Destructor will handle cleanup automatically
}


void RenderSystem::RegisterEditorMesh(Resource::Guid guid, std::shared_ptr<Mesh> mesh) {
	auto& renderSystem = Engine::GetRenderSystem();
	if (renderSystem.m_ResourceCache) {
		renderSystem.m_ResourceCache->RegisterEditorMesh(guid, mesh);
	} else {
		spdlog::error("RenderSystem: Cannot register editor mesh - ResourceCache not initialized");
	}
}

void RenderSystem::RegisterEditorMaterial(Resource::Guid guid, std::shared_ptr<Material> material) {
	auto& renderSystem = Engine::GetRenderSystem();
	if (renderSystem.m_ResourceCache) {
		renderSystem.m_ResourceCache->RegisterEditorMaterial(guid, material);
	} else {
		spdlog::error("RenderSystem: Cannot register editor material - ResourceCache not initialized");
	}
}

void RenderSystem::ClearEntityResources(uint64_t entityUID) {
	auto& renderSystem = Engine::GetRenderSystem();
	if (renderSystem.m_ResourceCache) {
		renderSystem.m_ResourceCache->ClearEntityResources(entityUID);
	} else {
		spdlog::error("RenderSystem: Cannot clear entity resources - ResourceCache not initialized");
	}
}

void RenderSystem::ClearAllEntityCaches() {
	auto& renderSystem = Engine::GetRenderSystem();
	if (renderSystem.m_ResourceCache) {
		renderSystem.m_ResourceCache->ClearAllEntityCaches();
	} else {
		spdlog::error("RenderSystem: Cannot clear entity caches - ResourceCache not initialized");
	}
}

// ========== Material Instance API Implementation ==========

std::shared_ptr<MaterialInstance> RenderSystem::GetMaterialInstance(
	uint64_t entityUID,
	std::shared_ptr<Material> baseMaterial)
{
	if (!m_MaterialInstanceManager) {
		spdlog::error("RenderSystem: MaterialInstanceManager not initialized");
		return nullptr;
	}

	return m_MaterialInstanceManager->GetMaterialInstance(entityUID, baseMaterial);
}

bool RenderSystem::HasMaterialInstance(uint64_t entityUID) const {
	if (!m_MaterialInstanceManager) {
		return false;
	}

	return m_MaterialInstanceManager->HasInstance(entityUID);
}

std::shared_ptr<MaterialInstance> RenderSystem::GetExistingMaterialInstance(uint64_t entityUID) const {
	if (!m_MaterialInstanceManager) {
		return nullptr;
	}

	return m_MaterialInstanceManager->GetExistingInstance(entityUID);
}

void RenderSystem::DestroyMaterialInstance(uint64_t entityUID) {
	if (!m_MaterialInstanceManager) {
		spdlog::warn("RenderSystem: MaterialInstanceManager not initialized");
		return;
	}

	m_MaterialInstanceManager->DestroyInstance(entityUID);
}

// ========== Material Property Block Management ==========

std::shared_ptr<MaterialPropertyBlock> RenderSystem::GetPropertyBlock(uint64_t entityUID) {
	// Check if property block already exists
	auto it = m_PropertyBlocks.find(entityUID);
	if (it != m_PropertyBlocks.end()) {
		return it->second;
	}

	// Create new property block
	auto propBlock = std::make_shared<MaterialPropertyBlock>();
	m_PropertyBlocks[entityUID] = propBlock;
	return propBlock;
}

bool RenderSystem::HasPropertyBlock(uint64_t entityUID) const {
	auto it = m_PropertyBlocks.find(entityUID);
	if (it == m_PropertyBlocks.end()) {
		return false;
	}

	// Check if property block has any properties set
	return !it->second->IsEmpty();
}

void RenderSystem::ClearPropertyBlock(uint64_t entityUID) {
	auto it = m_PropertyBlocks.find(entityUID);
	if (it != m_PropertyBlocks.end()) {
		it->second->Clear();
	}
}

void RenderSystem::DestroyPropertyBlock(uint64_t entityUID) {
	m_PropertyBlocks.erase(entityUID);
}

void RenderSystem::SetupDebugVisualization() {
	// Set up debug visualization meshes (required for DebugRenderPass)
	if (m_SceneRenderer && m_PrimitiveManager) {
		// Create debug visualization meshes using PrimitiveManager
		auto lightCube = m_PrimitiveManager->CreateDebugLightCube(5.0f);
		auto lightRay = m_PrimitiveManager->CreateDebugDirectionalRay(3.0f);
		auto wireframeCube = m_PrimitiveManager->CreateDebugWireframeCube(1.0f);

		// Debug visualization uses primitive shader (loaded by ShaderLibrary)
		m_SceneRenderer->SetDebugLightCubeMesh(lightCube);
		m_SceneRenderer->SetDebugDirectionalRayMesh(lightRay);
		m_SceneRenderer->SetDebugAABBWireframeMesh(wireframeCube);

		spdlog::info("Debug visualization meshes configured");
	} else {
		spdlog::error("RenderSystem: Cannot setup debug visualization - SceneRenderer or PrimitiveManager not initialized");
	}
}

// Resource type registration for Mesh (inline lambda to avoid file-scope variables)
REGISTER_RESOURCE_TYPE_SHARED_PTR(Mesh,
	[](const char* data)->std::shared_ptr<Mesh> {
		Resource::MeshAssetData dat = Resource::load_native_mesh_from_memory(data);
		std::vector<Texture> textures{};
		int i{};
		for (auto tex_guid : dat.textures) {
			Resource::TextureAssetData& tex = *ResourceRegistry::Instance().Get<Resource::TextureAssetData>(tex_guid);
			Texture texture;
			texture.id = TextureLoader::CreateGPUTextureCompressed(tex);
			texture.type = Resource::GetTextureTypeName(static_cast<Resource::TextureType>(dat.texture_type[i]));
			textures.emplace_back(texture);
			i++;
		}
		std::vector<Vertex> vert{};
		vert.resize(dat.vertices.size());
		memcpy(vert.data(), dat.vertices.data(), dat.vertices.size() * sizeof(Vertex));
		return std::make_shared<Mesh>(vert, dat.indices, textures);
	},
	[](std::shared_ptr<Mesh>&) {});

REGISTER_RESOURCE_TYPE_SHARED_PTR(Material,
	[](const char* data)->std::shared_ptr<Material> {
	Resource::MaterialAssetData dat = Resource::load_native_material_from_memory(data);
	Resource::ShaderAssetData shdr_dat = *ResourceRegistry::Instance().Get<Resource::ShaderAssetData>(dat.shader_guid);
	std::shared_ptr<Shader> shdr = Engine::GetRenderSystem().m_SceneRenderer->GetResourceManager()->LoadShader(shdr_dat.m_Name, shdr_dat.m_VertPath, shdr_dat.m_FragPath);
	std::shared_ptr<Material> mat = std::make_shared<Material>(shdr, dat.m_Name);
	mat->SetAlbedoColor(dat.m_AlbedoColor);
	mat->SetRoughnessValue(dat.m_RoughnessValue);
	mat->SetMetallicValue(dat.m_MetallicValue);
	return mat;
}, [](std::shared_ptr<Material>&) {});

//REGISTER_RESOURCE_TYPE(Texture, [](const char* data)->Texture {return Texture(); }, [](Texture&) {});

namespace Resource {
	REGISTER_RESOURCE_TYPE(ShaderAssetData, load_native_shader_from_memory, [](ShaderAssetData&) {});
	REGISTER_RESOURCE_TYPE(TextureAssetData, load_dds_texture_from_memory, [](TextureAssetData&) {});
}