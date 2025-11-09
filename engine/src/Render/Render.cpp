/******************************************************************************/
/*!
\file   render.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    definition of the render system

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

// Define implementation for tinyddsloader BEFORE any includes (header-only library like stb_image.h)
#define TINYDDSLOADER_IMPLEMENTATION

#include <Core/Window.h>
#include "Render/Render.h"
#include "Render/ShaderLibrary.hpp"
#include "Render/PrimitiveManager.hpp"
#include "Render/ComponentInitializer.hpp"
#include "Engine.hpp"  // For Engine::GetRenderSystem()
#include "components/transform.h"
#include "Manager/ResourceSystem.hpp"

#include "Messaging/Messaging_System.h"

#include <Resources/MaterialInstanceManager.h>
#include <Resources/MaterialInstance.h>
#include <Resources/MaterialPropertyBlock.h>
#include <Resources/Texture.h>

#include "native/native.h"
#include "native/texture.h"
#include "resource/utility.h"
#include "Render/Camera.h"

// Resource pipeline serialization
#include <rsc-core/serialization/serializer.hpp>

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
	if (m_ShaderLibrary->GetPointShadowShader()) {
		m_SceneRenderer->SetPointShadowShader(m_ShaderLibrary->GetPointShadowShader());
	}
	if (m_ShaderLibrary->GetShadowShader()) {
		m_SceneRenderer->SetSpotShadowShader(m_ShaderLibrary->GetShadowShader());
	}
	if (m_ShaderLibrary->GetPickingShader()) {
		m_SceneRenderer->SetPickingShader(m_ShaderLibrary->GetPickingShader());
	}
	if (m_ShaderLibrary->GetPrimitiveShader()) {
		m_SceneRenderer->SetDebugPrimitiveShader(m_ShaderLibrary->GetPrimitiveShader());
	}
	if (m_ShaderLibrary->GetOutlineShader()) {
		m_SceneRenderer->SetOutlineShader(m_ShaderLibrary->GetOutlineShader());
	}
	if (m_ShaderLibrary->GetSkyboxShader()) {
		m_SceneRenderer->SetSkyboxShader(m_ShaderLibrary->GetSkyboxShader());
	}
	if (m_ShaderLibrary->GetHDRComputeShader()) {
		m_SceneRenderer->SetHDRComputeShader(m_ShaderLibrary->GetHDRComputeShader());
	}
	if (m_ShaderLibrary->GetToneMappingShader()) {
		m_SceneRenderer->SetToneMappingShader(m_ShaderLibrary->GetToneMappingShader());
	}
	if (m_ShaderLibrary->GetParticleShader()) {
		m_SceneRenderer->SetParticleShader(m_ShaderLibrary->GetParticleShader());
	}
	// Editor resolve shader not needed - using simple glBlitFramebuffer instead
	// if (m_ShaderLibrary->GetEditorResolveShader()) {
	// 	m_SceneRenderer->SetEditorResolveShader(m_ShaderLibrary->GetEditorResolveShader());
	// }

	// Initialize primitive manager
	m_PrimitiveManager = std::make_unique<PrimitiveManager>();
	m_PrimitiveManager->Initialize();

	// Initialize material instance manager
	m_MaterialInstanceManager = std::make_unique<MaterialInstanceManager>();

	// Initialize component initializer (simplified, no longer needs subsystem references)
	m_ComponentInitializer = std::make_unique<ComponentInitializer>();
}

RenderSystem::~RenderSystem() {
	// Release subsystems in safe order
	m_ComponentInitializer.reset();
	m_MaterialInstanceManager.reset();
	m_PrimitiveManager.reset();
	m_ShaderLibrary.reset();
	m_SceneRenderer.reset();
	m_Camera.reset();
}

void RenderSystem::Init() {
	// Constructor already initialized everything, just setup debug visualization
	SetupDebugVisualization();

	spdlog::info("RenderSystem initialized");
	spdlog::info("RenderSystem: Call SetupComponentObservers() after world is created");
}

void RenderSystem::DisableHDRForEditor() {
	if (m_SceneRenderer) {
		m_SceneRenderer->ToggleHDRPipeline(false);
		spdlog::info("RenderSystem: HDR pipeline disabled for editor compatibility");
	} else {
		spdlog::error("RenderSystem: Cannot disable HDR - SceneRenderer not initialized");
	}
}

void RenderSystem::SetupComponentObservers(ecs::world& world) {
	if (m_ComponentInitializer) {
		m_ComponentInitializer->SetupObservers(world);
	} else {
		spdlog::error("RenderSystem: ComponentInitializer not initialized");
	}

	// Setup observer for component updates (triggered by registry.patch())
	entt::registry& registry = world.impl.get_registry();

	// Disconnect existing update observers first
	registry.on_update<MeshRendererComponent>().disconnect();

	// Connect update observer to sync material properties when component changes
	registry.on_update<MeshRendererComponent>().connect<&RenderSystem::OnMeshRendererUpdated>(this);

	spdlog::info("RenderSystem: MeshRendererComponent update observer registered");
}

void RenderSystem::Update(ecs::world& world) {
	PF_SYSTEM("GraphicSystem");

	auto world_camera = CameraSystem::GetActiveCamera();
	auto& frameData = m_SceneRenderer->GetFrameData();

	//update camera
	m_Camera->SetPosition(world_camera.m_Pos);
	glm::mat4 view = glm::lookAt(
		world_camera.m_Pos,
		world_camera.m_Pos + world_camera.m_Front,  // Look at position + front vector
		world_camera.m_Up
	);
	if (world_camera.m_Type == CameraComponent::CameraType::PERSPECTIVE) {
		m_Camera->SetPerspective(world_camera.m_Fov, world_camera.m_AspectRatio, world_camera.m_Near, world_camera.m_Far);
		frameData.projectionMatrix = m_Camera->GetProjectionMatrix();
		messagingSystem.Publish(MessageID::CAMERA_CALCULATION_UPDATE, std::make_unique<Camera_Calculation_Update>(frameData.viewMatrix,m_Camera->GetProjectionMatrix()));
	}

	frameData.viewMatrix = view;
	frameData.cameraPosition = world_camera.m_Pos;

	auto sceneObjects = world.filter_entities<MeshRendererComponent, TransformMtxComponent, VisibilityComponent>();
	auto sceneLights = world.filter_entities<LightComponent, TransformComponent>();

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
		auto [mesh, transform, visible] {obj.get<MeshRendererComponent, TransformMtxComponent, VisibilityComponent>()};
		const uint64_t entityUID = obj.get_uid();

		// === RESOURCE LOOKUP (ON-DEMAND LOADING FROM ResourceRegistry) ===

		// Load mesh resource (from ResourceRegistry or PrimitiveManager)
		auto meshResource = LoadMeshResource(mesh);

		auto materialLoader = [this, hasAttachedMat = mesh.hasAttachedMaterial, entityUID](rp::BasicIndexedGuid biguid) {
			return LoadMaterialResource(static_cast<rp::TypeNameGuid<"material">&>(biguid), hasAttachedMat, entityUID);
			};

		bool isMeshValid{ false };
		std::visit([&isMeshValid](auto& ptr) {isMeshValid = static_cast<bool>(ptr); }, meshResource);

		// Skip if resources failed to load
		if (!isMeshValid) {
				static std::unordered_set<uint64_t> warnedEntities;
			if (warnedEntities.find(entityUID) == warnedEntities.end()) {
				spdlog::warn("RenderSystem: Failed to load resources for entity {}", entityUID);
				warnedEntities.insert(entityUID);
			}
			continue;
		}

		// NOTE: Material system follows Unity-style behavior:
		// - Component properties: Used for editor/serialization (synced to base material)
		// - Material instances: Runtime copy-on-write for per-entity customization

		const auto shared_ptr_visitor = [&](std::shared_ptr<Mesh>& meshResourcePtr, std::shared_ptr<Material> const& materialResourcePtr)
			// Render the entity
			{
				// Material customization is now handled by MaterialPropertyBlocks
				// MaterialOverridesSystem creates property blocks from MaterialOverridesComponent
				// Property blocks are applied by SceneRenderer after base material

				RenderableData renderData;
				renderData.mesh = meshResourcePtr;
				renderData.material = materialResourcePtr;
				renderData.transform = transform.m_Mtx;
				renderData.visible = visible.m_IsVisible;
				renderData.renderLayer = 1;

				// Use existing entityUID (uint64_t) and cast to uint32_t for objectID
				renderData.objectID = static_cast<uint32_t>(entityUID) & rp::lo_bitmask32_v<23>;

				// Attach property block if it exists and has properties
				auto propBlockIt = m_PropertyBlocks.find(entityUID);
				if (propBlockIt != m_PropertyBlocks.end() && !propBlockIt->second->IsEmpty()) {
					renderData.propertyBlock = propBlockIt->second;

					// Debug: Log property block attachment
					static int propBlockDebugCount = 0;
					if (propBlockDebugCount < 5) {
						spdlog::info("RenderSystem: Attaching property block for entity {} (properties: {})",
							entityUID, propBlockIt->second->GetPropertyCount());
						propBlockDebugCount++;
					}
				}

				// Debug: Log entity UID assignment for first few entities
				static int debugCount = 0;
				if (debugCount < 5) {
					spdlog::info("RenderSystem: Entity UID assignment - Entity: {}, UID: {}, static_cast result: {}",
						debugCount, entityUID, static_cast<uint32_t>(entityUID));
					debugCount++;
				}

				// Assert entity ID validity for debugging picking
				//assert(renderData.objectID != 0 && "Entity UID should not be zero for picking to work");
				assert(renderData.objectID < 16777215 && "Entity UID exceeds 24-bit limit for picking system"); // 24-bit max for RGB encoding

				// Assert mesh validity for rendering
				assert(meshResourcePtr->GetVertexArray() && "Mesh must have valid VAO for rendering");
				assert(meshResourcePtr->GetVertexArray()->GetVAOHandle() != 0 && "Mesh VAO must be bound to valid OpenGL handle");
				assert(!meshResourcePtr->vertices.empty() && "Mesh must have vertices for rendering");

				m_SceneRenderer->SubmitRenderable(renderData);

				// Submit AABB for debug visualization (using pre-calculated mesh AABB)
				if (visible.m_IsVisible && meshResourcePtr->GetAABB().IsValid()) {
					DebugAABB debugAABB(meshResourcePtr->GetAABB(), transform.m_Mtx, glm::vec3(1.0f, 0.0f, 0.0f));
					frameData.debugAABBs.push_back(debugAABB);
				}
			};
		std::visit([&](auto&& var) {
			using Type = std::remove_pointer_t<std::remove_cvref_t<decltype(var)>>;
			if constexpr (std::is_same_v<Type, std::shared_ptr<Mesh>>) {
				if (mesh.m_MaterialGuid.find("unnamed slot") == mesh.m_MaterialGuid.end() || mesh.m_MaterialGuid.size() != 1) {
					mesh.m_MaterialGuid.clear();
					mesh.m_MaterialGuid.emplace("unnamed slot", static_cast<rp::BasicIndexedGuid>(rp::TypeNameGuid<"material">{}));
				}
				shared_ptr_visitor(var, materialLoader(mesh.m_MaterialGuid.begin()->second));
			}
			else {
				// Multi-slot mesh: match materials by slot name to preserve user assignments
				// Only add missing slots, never clear existing assignments
				for (auto& [slotName, meshPtr] : *var) {
					// Check if this slot exists in the component's material map
					auto matIt = mesh.m_MaterialGuid.find(slotName);

					if (matIt == mesh.m_MaterialGuid.end()) {
						// Slot doesn't exist - add it with null GUID (will use default material)
						mesh.m_MaterialGuid.emplace(slotName, static_cast<rp::BasicIndexedGuid>(rp::TypeNameGuid<"material">{}));
						matIt = mesh.m_MaterialGuid.find(slotName);
					}

					// Submit renderable with material from this slot
					shared_ptr_visitor(meshPtr, materialLoader(matIt->second));
				}

				// Clean up slots that don't exist in mesh anymore (optional)
				// This prevents accumulation of unused slots when mesh changes
				std::vector<std::string> slotsToRemove;
				for (const auto& [componentSlotName, guid] : mesh.m_MaterialGuid) {
					bool slotExistsInMesh = false;
					for (const auto& [meshSlotName, ptr] : *var) {
						if (meshSlotName == componentSlotName) {
							slotExistsInMesh = true;
							break;
						}
					}
					if (!slotExistsInMesh) {
						slotsToRemove.push_back(componentSlotName);
					}
				}
				for (const auto& slotName : slotsToRemove) {
					mesh.m_MaterialGuid.erase(slotName);
				}
			}
			}, meshResource);
	}

	for (auto light : sceneLights) {
		auto [lightComponent, position] {light.get<LightComponent, TransformComponent>()};
		SubmittedLightData lightData;
		lightData.type = lightComponent.m_Type;
		lightData.color = lightComponent.m_Color;
		lightData.direction = lightComponent.m_Direction;
		lightData.enabled = lightComponent.m_IsEnabled;
		lightData.castShadows = lightComponent.m_CastShadows;
		lightData.innerCone = lightComponent.m_InnerCone;
		lightData.outerCone = lightComponent.m_OuterCone;
		lightData.diffuseIntensity = lightComponent.m_Intensity;
		lightData.range = lightComponent.m_Range;
		lightData.position = position.m_Translation;
		m_SceneRenderer->SubmitLight(lightData);
	}

	//render frame
	m_SceneRenderer->Render();

	//clear frame data AFTER rendering (so particles submitted before render are included)
	m_SceneRenderer->ClearFrame();
}
void RenderSystem::FixedUpdate(ecs::world& w) {
	Update(w);
}
void RenderSystem::Exit() {
	// Clear all material instances
	if (m_MaterialInstanceManager) {
		m_MaterialInstanceManager->ClearAllInstances();
	}

	// Clear all property blocks
	m_PropertyBlocks.clear();

	// Clear cached default material
	m_DefaultMaterial.reset();

	// Destructor will handle cleanup automatically
}

bool RenderSystem::RegisterEditorMesh(rp::Guid guid, std::shared_ptr<Mesh> mesh) {
	return ResourceRegistry::Instance().RegisterInMemory(guid, mesh);
}

bool RenderSystem::RegisterEditorMaterial(rp::Guid guid, std::shared_ptr<Material> material) {
	return ResourceRegistry::Instance().RegisterInMemory(guid, material);
}

void RenderSystem::SyncMaterialFromComponent(uint64_t entityUID, const MeshRendererComponent& /*meshRenderer*/) {
	auto& renderSystem = Engine::GetRenderSystem();

	// When material GUID changes, destroy cached material instance
	// This forces reload of the new material on next render
	if (renderSystem.m_MaterialInstanceManager && renderSystem.m_MaterialInstanceManager->HasInstance(entityUID)) {
		spdlog::debug("RenderSystem: Material GUID changed for entity {}, clearing cached instance", entityUID);
		renderSystem.m_MaterialInstanceManager->DestroyInstance(entityUID);
	}
}

void RenderSystem::OnMeshRendererUpdated(entt::registry& registry, entt::entity entity) {
	// Get the updated component
	auto* meshComp = registry.try_get<MeshRendererComponent>(entity);
	if (!meshComp) return;

	// Automatically update hasAttachedMaterial based on material GUID validity
	meshComp->hasAttachedMaterial = (meshComp->m_MaterialGuid.begin()->second.m_guid != rp::null_guid);

	// Get entity UID
	const uint64_t entityUID = static_cast<uint64_t>(ecs::world::detail::entity_id_cast(entity));

	// Sync to cached material
	SyncMaterialFromComponent(entityUID, *meshComp);

	// Force renderer to rebuild cached instance data
	// This ensures mesh/material changes are reflected immediately in the viewport
	if (m_SceneRenderer) {
		m_SceneRenderer->ForceRebuildInstanceCache();
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

// ========== Resource Loading Helpers ==========

std::variant<std::shared_ptr<Mesh>, std::vector<std::pair<std::string, std::shared_ptr<Mesh>>>*> RenderSystem::LoadMeshResource(const MeshRendererComponent& meshComp) const {
	// Handle primitives via PrimitiveManager
	if (meshComp.isPrimitive) {
		static int primLoadLogCount = 0;
		if (primLoadLogCount < 20) {
			spdlog::warn("LoadMeshResource: isPrimitive=true, returning primitive mesh (primitiveType: {})", static_cast<int>(meshComp.m_PrimitiveType));
			primLoadLogCount++;
		}
		switch (meshComp.m_PrimitiveType) {
		case MeshRendererComponent::PrimitiveType::CUBE:
			return m_PrimitiveManager->GetSharedCubeMesh();
		case MeshRendererComponent::PrimitiveType::PLANE:
			return m_PrimitiveManager->GetSharedPlaneMesh();
		default:
			spdlog::warn("RenderSystem: Invalid primitive type");
			return nullptr;
		}
	}

	// Check for null GUID
	if (meshComp.m_MeshGuid.m_guid == rp::null_guid) {
		return nullptr;
	}

	// Check if already loaded (from file OR in-memory)
	auto& registry = ResourceRegistry::Instance();
	//Handle meshHandle = registry.Find<std::shared_ptr<Mesh>>(meshComp.m_MeshGuid.m_guid);
	//
	//if (meshHandle) {  // Handle has operator bool()
	//	// Already loaded (either from file or RegisterInMemory)
	//	auto* pool = registry.Pool<std::shared_ptr<Mesh>>();
	//	if (pool) {
	//		auto* meshPtr = pool->Ptr(meshHandle);
	//		if (meshPtr) return *meshPtr;
	//	}
	//}
	Handle meshHandle{};
	auto* ptr = registry.Get<std::vector<std::pair<std::string, std::shared_ptr<Mesh>>>>(meshComp.m_MeshGuid.m_guid, &meshHandle);

	if (ptr && !ptr->empty()) {
		return ptr;
	}

	// Not found - mesh was not loaded from file and not registered in-memory
	static std::unordered_set<std::string> warnedMeshGuids;
	std::string guidStr = meshComp.m_MeshGuid.m_guid.to_hex();
	if (warnedMeshGuids.find(guidStr) == warnedMeshGuids.end()) {
		spdlog::warn("RenderSystem: Mesh GUID {} not found (no file or in-memory registration).", guidStr);
		warnedMeshGuids.insert(guidStr);
	}

	return nullptr;
}

std::shared_ptr<Material> RenderSystem::LoadMaterialResource(
	const rp::TypeNameGuid<"material">& materialGuid,
	bool hasAttachedMaterial,
	uint64_t entityUID) const
{
	// Check for null GUID
	if (materialGuid.m_guid == rp::null_guid || !hasAttachedMaterial) {
		// No material attached - use cached default material
		// This ensures stable memory address across frames for proper batching
		if (!m_DefaultMaterial) {
			auto pbrShader = m_ShaderLibrary->GetPBRShader();
			if (!pbrShader) {
				spdlog::error("RenderSystem: PBR shader not available for default material");
				return nullptr;
			}
			m_DefaultMaterial = std::make_shared<Material>(pbrShader, "DefaultMaterial_Shared");
			spdlog::info("RenderSystem: Created cached default material (shared across all entities with no attached material)");
		}
		static int defaultMatLogCount = 0;
		if (defaultMatLogCount < 20) {
			spdlog::info("Entity {}: Using default material (null GUID or hasAttachedMaterial=false)", entityUID);
			defaultMatLogCount++;
		}
		return m_DefaultMaterial;
	}

	// Check if already loaded OR load from disk via ResourceSystem::FileEntries
	auto& registry = ResourceRegistry::Instance();
	auto* matPtr = registry.Get<std::shared_ptr<Material>>(materialGuid.m_guid);

	if (matPtr)
	{
		return *matPtr;
	}

	// Not found - material was not loaded from file and not registered in-memory
	// Create default material as fallback
	std::string guidStr = materialGuid.m_guid.to_hex();
	spdlog::error("LoadMaterialResource: Material GUID {} NOT FOUND in registry after Get() call! (entity {})",
	              guidStr.substr(0, 8), entityUID);

	auto pbrShader = m_ShaderLibrary->GetPBRShader();
	if (!pbrShader) {
		spdlog::error("RenderSystem: PBR shader not available for fallback material");
		return nullptr;
	}
	return std::make_shared<Material>(pbrShader, "FallbackMaterial_" + guidStr);
}

std::vector<std::pair<std::string, std::shared_ptr<Mesh>>> LoadMeshFromResource(const char* data) {
	MeshResourceData dat = rp::serialization::serializer<"bin">::deserialize<MeshResourceData>(reinterpret_cast<const std::byte*>(data));
	std::vector<std::pair<std::string, std::shared_ptr<Mesh>>> meshes;
	for (const auto& mesh : dat.meshes) {
		std::vector<Vertex> vert{}; vert.resize(mesh.vertices.size());
		for (size_t i = 0; i < mesh.vertices.size(); ++i) {
			vert[i].Position = mesh.vertices[i].Position;
			vert[i].Normal = mesh.vertices[i].Normal;
			vert[i].TexCoords = mesh.vertices[i].TexCoords;
			vert[i].Tangent = mesh.vertices[i].Tangent;
			vert[i].Bitangent = mesh.vertices[i].Bitangent;
		}
		std::vector<unsigned int> indices{};
		for (const auto& matslot : mesh.materials) {
			indices.resize(matslot.index_count);
			for (unsigned int i{}; i < matslot.index_count; i++) {
				indices[i] = mesh.indices[i + matslot.index_begin];
			}
			meshes.emplace_back(std::pair<std::string, std::shared_ptr<Mesh>>(matslot.material_slot_name, std::make_shared<Mesh>(vert, indices, std::vector<Texture>{})));
		}
	}
	return meshes;
	}

void UnloadMeshFromResource(std::vector<std::pair<std::string, std::shared_ptr<Mesh>>>&) {}

using Meshes = std::vector<std::pair<std::string, std::shared_ptr<Mesh>>>;

// ========== Resource Type Registrations ==========
REGISTER_RESOURCE_TYPE_ALIASE(Meshes, mesh, LoadMeshFromResource, UnloadMeshFromResource)

REGISTER_RESOURCE_TYPE_ALIASE(std::shared_ptr<Material>, material,
	[](const char* data)->std::shared_ptr<Material> {
		// Deserialize MaterialResourceData from binary
		MaterialResourceData matData = rp::serialization::serializer<"bin">::deserialize<MaterialResourceData>(
			reinterpret_cast<const std::byte*>(data)
		);

		// Get RenderSystem to access shader library and resource manager
		auto& renderSystem = Engine::GetRenderSystem();
		auto sceneRenderer = renderSystem.GetSceneRenderer();
		auto resourceManager = sceneRenderer->GetResourceManager();

		// Load shader - use default PBR shader if material uses standard shaders
		std::shared_ptr<Shader> shader;

		// Check if material uses default PBR shaders (avoid duplicate loading)
		// Handle both "main_pbr" and "main_pbr.vert"/"main_pbr.frag"
		bool usesPBRShader = (matData.vert_name == "main_pbr" || matData.vert_name == "main_pbr.vert") &&
		                     (matData.frag_name == "main_pbr" || matData.frag_name == "main_pbr.frag");

		if (usesPBRShader) {
			// Use the pre-loaded PBR shader from ShaderLibrary (no duplicate loading)
			shader = renderSystem.GetShaderLibrary()->GetPBRShader();
			if (!shader) {
				spdlog::error("PBR shader not available for material '{}'", matData.material_name);
				return nullptr;
			}
			spdlog::debug("Material '{}': Using default PBR shader (no duplicate loading)", matData.material_name);
		} else {
			// Custom shaders - load separately
			std::string vertPath = "assets/shaders/" + matData.vert_name;
			std::string fragPath = "assets/shaders/" + matData.frag_name;

			// Ensure .vert and .frag extensions
			if (vertPath.find(".vert") == std::string::npos) {
				vertPath += ".vert";
			}
			if (fragPath.find(".frag") == std::string::npos) {
				fragPath += ".frag";
			}

			shader = resourceManager->LoadShader(matData.material_name, vertPath, fragPath);
			if (!shader) {
				spdlog::error("Failed to load custom shader for material '{}': {} / {}",
							 matData.material_name, vertPath, fragPath);
				// Fall back to default PBR shader
				shader = renderSystem.GetShaderLibrary()->GetPBRShader();
				if (!shader) {
					spdlog::error("No fallback shader available for material '{}'", matData.material_name);
					return nullptr;
				}
			}
		}

		// Create Material instance
		auto material = std::make_shared<Material>(shader, matData.material_name);

		// Apply PBR base properties
		material->SetAlbedoColor(matData.albedo);
		material->SetMetallicValue(matData.metallic);
		material->SetRoughnessValue(matData.roughness);

		// Apply extended float properties
		for (const auto& [name, value] : matData.float_properties) {
			material->SetFloat(name, value);
		}

		// Apply extended vec3 properties
		for (const auto& [name, value] : matData.vec3_properties) {
			material->SetVec3(name, value);
		}

		// Apply extended vec4 properties
		for (const auto& [name, value] : matData.vec4_properties) {
			material->SetVec4(name, value);
		}

		// Load and apply textures from texture_properties (GUID map)
		auto& registry = ResourceRegistry::Instance();
		int textureUnit = 0;
		for (const auto& [uniform_name, texture_guid] : matData.texture_properties) {
			// Skip null GUIDs
			if (texture_guid == rp::null_guid) {
				continue;
			}

			// Load texture from ResourceRegistry (synchronous)
			auto* texturePtr = registry.Get<std::shared_ptr<Texture>>(texture_guid);
			if (texturePtr && *texturePtr) {
				material->SetTexture(uniform_name, *texturePtr, textureUnit);
				spdlog::info("Material '{}': Loaded texture '{}' (GUID: {}, GPU ID: {})",
					matData.material_name, uniform_name, texture_guid.to_hex().substr(0, 8), (*texturePtr)->id);
				++textureUnit;
			}
			else {
				spdlog::warn("Material '{}': Failed to load texture '{}' (GUID: {})",
					matData.material_name, uniform_name, texture_guid.to_hex().substr(0, 8));
			}
		}

		// Apply blend mode (0 = Opaque, 1 = Transparent)
		material->SetBlendMode(static_cast<BlendingMode>(matData.blend_mode));

		spdlog::info("Successfully loaded material '{}' from resource pipeline ({} textures)",
			matData.material_name, textureUnit);
		return material;
	},
	[](std::shared_ptr<Material>&) {
		// Cleanup - Material destructor handles GPU resource cleanup
		// No additional cleanup needed
	});

// Register Texture resource type
REGISTER_RESOURCE_TYPE_ALIASE(std::shared_ptr<Texture>, texture,
	[](const char* data) -> std::shared_ptr<Texture> {
		// Deserialize TextureResourceData from binary
		TextureResourceData texData = rp::serialization::serializer<"bin">::deserialize<TextureResourceData>(
			reinterpret_cast<const std::byte*>(data)
		);

		// Load DDS file from blob
		tinyddsloader::DDSFile ddsFile;
		auto result = ddsFile.Load(reinterpret_cast<const uint8_t*>(texData.m_TexData.Raw()), texData.m_TexData.Size());
		if (result != tinyddsloader::Result::Success) {
			spdlog::error("Failed to load DDS texture from binary data");
			return nullptr;
		}

		// Create GPU texture from DDS
		unsigned int textureID = TextureLoader::CreateGPUTextureCompressed(ddsFile);
		if (textureID == 0) {
			spdlog::error("Failed to create GPU texture from DDS data");
			return nullptr;
		}

		// Create Texture object
		auto texture = std::make_shared<Texture>();
		texture->id = textureID;
		texture->type = "texture_diffuse";  // Default type
		texture->path = "";  // No file path for loaded resources
		texture->target = GL_TEXTURE_2D;

		spdlog::info("Successfully loaded texture from resource pipeline (GPU ID: {})", textureID);
		return texture;
	},
	[](std::shared_ptr<Texture>& texture) {
		// Cleanup GPU texture
		if (texture && texture->id != 0) {
			glDeleteTextures(1, &texture->id);
			texture->id = 0;
		}
	});

// Note: ShaderAssetData registration removed
// Shaders are now loaded by name from MaterialResourceData