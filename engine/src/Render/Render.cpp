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

#include <Core/Window.h>
#include "Render/Render.h"
#include "Render/ShaderLibrary.hpp"
#include "Render/PrimitiveManager.hpp"
#include "Render/ComponentInitializer.hpp"
#include "Engine.hpp"  // For Engine::GetRenderSystem()
#include "components/transform.h"
#include "Manager/ResourceSystem.hpp"

#include <Resources/MaterialInstanceManager.h>
#include <Resources/MaterialInstance.h>
#include <Resources/MaterialPropertyBlock.h>

#include "native/native.h"
#include "Render/Camera.h"

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

	//begin frame
	m_SceneRenderer->ClearFrame();

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
		std::shared_ptr<Mesh> meshResource = LoadMeshResource(mesh);

		// Load material resource (from ResourceRegistry or create default)
		std::shared_ptr<Material> materialResource = LoadMaterialResource(
			mesh.m_MaterialGuid,
			mesh.hasAttachedMaterial,
			entityUID
		);

		// Skip if resources failed to load
		if (!meshResource || !materialResource) {
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

		// Render the entity
		{
			// Material customization is now handled by MaterialPropertyBlocks
			// MaterialOverridesSystem creates property blocks from MaterialOverridesComponent
			// Property blocks are applied by SceneRenderer after base material

			RenderableData renderData;
			renderData.mesh = meshResource;
			renderData.material = materialResource;
			renderData.transform = transform.m_Mtx;
			renderData.visible = visible.m_IsVisible;
			renderData.renderLayer = 1;

			// Use existing entityUID (uint64_t) and cast to uint32_t for objectID
			renderData.objectID = static_cast<uint32_t>(entityUID);

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
			assert(renderData.objectID != 0 && "Entity UID should not be zero for picking to work");
			assert(renderData.objectID < 16777215 && "Entity UID exceeds 24-bit limit for picking system"); // 24-bit max for RGB encoding

			// Assert mesh validity for rendering
			assert(meshResource->GetVertexArray() && "Mesh must have valid VAO for rendering");
			assert(meshResource->GetVertexArray()->GetVAOHandle() != 0 && "Mesh VAO must be bound to valid OpenGL handle");
			assert(!meshResource->vertices.empty() && "Mesh must have vertices for rendering");

			m_SceneRenderer->SubmitRenderable(renderData);

			// Submit AABB for debug visualization (using pre-calculated mesh AABB)
			if (visible.m_IsVisible && meshResource->GetAABB().IsValid()) {
				DebugAABB debugAABB(meshResource->GetAABB(), transform.m_Mtx, glm::vec3(1.0f, 0.0f, 0.0f));
				frameData.debugAABBs.push_back(debugAABB);
			}
		}
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
		lightData.diffuseIntensity = lightComponent.m_DiffuseIntensity;
		lightData.ambientIntensity = lightComponent.m_AmbientIntensity;
		lightData.range = lightComponent.m_Range;
		lightData.position = position.m_Translation;
		m_SceneRenderer->SubmitLight(lightData);
	}

	//render frame
	m_SceneRenderer->Render();
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

	// Destructor will handle cleanup automatically
}

bool RenderSystem::RegisterEditorMesh(rp::Guid guid, std::shared_ptr<Mesh> mesh) {
	return ResourceRegistry::Instance().RegisterInMemory(guid, mesh);
}

bool RenderSystem::RegisterEditorMaterial(rp::Guid guid, std::shared_ptr<Material> material) {
	return ResourceRegistry::Instance().RegisterInMemory(guid, material);
}

void RenderSystem::SyncMaterialFromComponent(uint64_t entityUID, const MeshRendererComponent& meshRenderer) {
	auto& renderSystem = Engine::GetRenderSystem();

	// NOTE: Material sync is now handled by MaterialOverridesSystem
	// MeshRendererComponent no longer has embedded material properties
	// Material customization is done via MaterialOverridesComponent → MaterialPropertyBlock

	spdlog::debug("RenderSystem: SyncMaterialFromComponent called for entity {} (no-op, handled by MaterialOverridesSystem)", entityUID);
}

void RenderSystem::OnMeshRendererUpdated(entt::registry& registry, entt::entity entity) {
	// Get the updated component
	auto* meshComp = registry.try_get<MeshRendererComponent>(entity);
	if (!meshComp) return;

	// Get entity UID
	const uint64_t entityUID = static_cast<uint64_t>(ecs::world::detail::entity_id_cast(entity));

	// Sync to cached material
	SyncMaterialFromComponent(entityUID, *meshComp);
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

std::shared_ptr<Mesh> RenderSystem::LoadMeshResource(const MeshRendererComponent& meshComp) const {
	// Handle primitives via PrimitiveManager
	if (meshComp.isPrimitive) {
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
	if (meshComp.m_MeshGuid == Resource::null_guid) {
		return nullptr;
	}

	// Check if already loaded (from file OR in-memory)
	auto& registry = ResourceRegistry::Instance();
	Handle meshHandle = registry.Find<std::shared_ptr<Mesh>>(meshComp.m_MeshGuid);

	if (meshHandle) {  // Handle has operator bool()
		// Already loaded (either from file or RegisterInMemory)
		auto* pool = registry.Pool<std::shared_ptr<Mesh>>();
		if (pool) {
			auto* meshPtr = pool->Ptr(meshHandle);
			if (meshPtr) return *meshPtr;
		}
	}

	// Not found - mesh was not loaded from file and not registered in-memory
	// This means the editor created a mesh but didn't call RegisterEditorMesh()
	static std::unordered_set<std::string> warnedMeshGuids;
	std::string guidStr = meshComp.m_MeshGuid.to_hex();
	if (warnedMeshGuids.find(guidStr) == warnedMeshGuids.end()) {
		spdlog::warn("RenderSystem: Mesh GUID {} not found (no file or in-memory registration). "
			"If this is an editor-created mesh, call RenderSystem::RegisterEditorMesh().", guidStr);
		warnedMeshGuids.insert(guidStr);
	}

	return nullptr;
}

std::shared_ptr<Material> RenderSystem::LoadMaterialResource(const Resource::Guid& materialGuid, bool hasAttachedMaterial, uint64_t entityUID) const {
	// Check for null GUID
	if (materialGuid == Resource::null_guid || !hasAttachedMaterial) {
		// No material attached - create default
		auto pbrShader = m_ShaderLibrary->GetPBRShader();
		if (!pbrShader) {
			spdlog::error("RenderSystem: PBR shader not available for default material");
			return nullptr;
		}
		return std::make_shared<Material>(pbrShader, "DefaultMaterial_Entity_" + std::to_string(entityUID));
	}

	// Check if already loaded (from file OR in-memory via RegisterInMemory)
	auto& registry = ResourceRegistry::Instance();
	Handle matHandle = registry.Find<std::shared_ptr<Material>>(materialGuid);

	if (matHandle) {  // Handle has operator bool()
		// Already loaded (either from file or RegisterInMemory)
		auto* pool = registry.Pool<std::shared_ptr<Material>>();
		if (pool) {
			auto* matPtr = pool->Ptr(matHandle);
			if (matPtr) return *matPtr;
		}
	}

	// Not found - material was not loaded from file and not registered in-memory
	// This means the editor created a material but didn't call RegisterEditorMaterial()
	// Create default material as fallback
	static std::unordered_set<std::string> warnedGuids;
	std::string guidStr = materialGuid.to_hex();
	if (warnedGuids.find(guidStr) == warnedGuids.end()) {
		spdlog::warn("RenderSystem: Material GUID {} not found (no file or in-memory registration). Using default material. "
			"If this is an editor-created material, call RenderSystem::RegisterEditorMaterial().", guidStr);
		warnedGuids.insert(guidStr);
	}

	auto pbrShader = m_ShaderLibrary->GetPBRShader();
	if (!pbrShader) {
		spdlog::error("RenderSystem: PBR shader not available for fallback material");
		return nullptr;
	}
	return std::make_shared<Material>(pbrShader, "FallbackMaterial_" + guidStr);
}

// ========== Resource Type Registrations ==========

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