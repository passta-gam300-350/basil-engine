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
#include "Engine.hpp"
//#include "components/transform.h"
#include "Manager/ResourceSystem.hpp"

#include "native/loader.h"
#include "native/texture.h"
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

// Editor resource caches at file scope
namespace {
	std::unordered_map<Resource::Guid, std::shared_ptr<Mesh>> g_EditorMeshCache;
	std::unordered_map<Resource::Guid, std::shared_ptr<Material>> g_EditorMaterialCache;
}

// Static PBR shader storage for scene objects
std::shared_ptr<Shader> RenderSystem::s_CubeShader = nullptr;

void RenderSystem::InstanceData::Acquire() {
	m_SceneRenderer = std::make_unique<SceneRenderer>();
	m_Camera = std::make_unique<Camera>(CameraType::Perspective);
	m_Camera->SetPerspective(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
	m_Camera->SetPosition(glm::vec3(0.0f, 2.0f, 8.0f));
	m_Camera->SetRotation(glm::vec3(-10.0f, 0.0f, 0.0f));
}

void RenderSystem::InstanceData::Release() {
	m_SceneRenderer.reset();
	m_Camera.reset();
}

void RenderSystem::Init() {
	Instance().Acquire();

	// Load shaders and setup debug visualization meshes only
	LoadBasicShaders();
	SetupDebugVisualization();

	spdlog::info("RenderSystem initialized");



}

void RenderSystem::Update(ecs::world& world) {
	InstanceData& inst{ Instance() };

	PF_SYSTEM("GraphicSystem");

	//begin frame
	inst.m_SceneRenderer->ClearFrame();

	auto world_camera = CameraSystem::GetActiveCamera();
	auto& frameData = inst.m_SceneRenderer->GetFrameData();

	//update camera
	inst.m_Camera->SetPosition(world_camera.m_Pos);
	glm::mat4 view = glm::lookAt(
		world_camera.m_Pos,
		world_camera.m_Pos + world_camera.m_Front,  // Look at position + front vector
		world_camera.m_Up
	);
	if (world_camera.m_Type == CameraComponent::CameraType::PERSPECTIVE) {
		inst.m_Camera->SetPerspective(world_camera.m_Fov, world_camera.m_AspectRatio, world_camera.m_Near, world_camera.m_Far);
		frameData.projectionMatrix = inst.m_Camera->GetProjectionMatrix();
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

		std::shared_ptr<Mesh> meshResource;
		std::shared_ptr<Material> materialResource;



		// Try editor cache first (for runtime-created assets)
		if (g_EditorMeshCache.contains(mesh.m_MeshGuid)) {
			meshResource = g_EditorMeshCache[mesh.m_MeshGuid];
		}
		else if (mesh.isPrimitive) {


			switch (mesh.m_PrimitiveType) {
			case MeshRendererComponent::PrimitiveType::CUBE:
				meshResource = GetSharedCubeMesh();
				assert(meshResource && "Shared cube mesh must be available");
				mesh.m_MeshGuid = Resource::Guid::generate();
				RegisterEditorMesh(mesh.m_MeshGuid, meshResource);
				break;
			case MeshRendererComponent::PrimitiveType::PLANE:
				meshResource = GetSharedPlaneMesh();
				assert(meshResource && "Shared plane mesh must be available");
				mesh.m_MeshGuid = Resource::Guid::generate();
				RegisterEditorMesh(mesh.m_MeshGuid, meshResource);
				break;
			default:
				assert(false && "Invalid GUID");


			}

		}
		else {
			// Fall back to file-based registry
			auto* meshPtr = ResourceRegistry::Instance().Get<std::shared_ptr<Mesh>>(mesh.m_MeshGuid);
			if (meshPtr) {
				meshResource = *meshPtr;
			}
		}

		if (g_EditorMaterialCache.contains(mesh.m_MaterialGuid)) {
			materialResource = g_EditorMaterialCache[mesh.m_MaterialGuid];
		}
		else if (!mesh.hasAttachedMaterial)
		{
			assert(s_CubeShader && "PBR shader must be available");

			auto MeshMaterial = std::make_shared<Material>(s_CubeShader, "SCubeShaderDefault_" + std::to_string(obj.get_uid()));
			assert(MeshMaterial && "Material creation failed");
			MeshMaterial->SetAlbedoColor(mesh.material.m_AlbedoColor);
			MeshMaterial->SetMetallicValue(mesh.material.metallic);
			MeshMaterial->SetRoughnessValue(mesh.material.roughness);

			mesh.m_MaterialGuid = Resource::Guid::generate();
			RegisterEditorMaterial(mesh.m_MaterialGuid, MeshMaterial);
			materialResource = MeshMaterial;
		}
		else {
			// Fall back to file-based registry
			auto* materialPtr = ResourceRegistry::Instance().Get<std::shared_ptr<Material>>(mesh.m_MaterialGuid);
			if (materialPtr) {
				materialResource = *materialPtr;
			}

		}

		// Before render update material properties if needed
		if (materialResource)
		{
			materialResource->SetAlbedoColor(mesh.material.m_AlbedoColor);
		}

		// Only render if we have both mesh and material
		if (meshResource && materialResource) {
			RenderableData renderData;
			renderData.mesh = meshResource;
			renderData.material = materialResource;
			renderData.transform = transform.m_Mtx;
			renderData.visible = visible.m_IsVisible;
			renderData.renderLayer = 1;

			uint32_t entityUID = static_cast<uint32_t>(obj.get_uid());
			renderData.objectID = entityUID;

			// Debug: Log entity UID assignment for first few entities
			static int debugCount = 0;
			if (debugCount < 5) {
				spdlog::info("RenderSystem: Entity UID assignment - Entity: {}, UID: {}, static_cast result: {}",
					debugCount, obj.get_uid(), entityUID);
				debugCount++;
			}

			// Assert entity ID validity for debugging picking
			assert(entityUID != 0 && "Entity UID should not be zero for picking to work");
			assert(entityUID < 16777215 && "Entity UID exceeds 24-bit limit for picking system"); // 24-bit max for RGB encoding

			// Assert mesh validity for rendering
			assert(meshResource->GetVertexArray() && "Mesh must have valid VAO for rendering");
			assert(meshResource->GetVertexArray()->GetVAOHandle() != 0 && "Mesh VAO must be bound to valid OpenGL handle");
			assert(!meshResource->vertices.empty() && "Mesh must have vertices for rendering");

			inst.m_SceneRenderer->SubmitRenderable(renderData);

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
		lightData.innerCone = lightComponent.m_InnerCone;
		lightData.outerCone = lightComponent.m_OuterCone;
		lightData.intensity = lightComponent.m_Intensity;
		lightData.range = lightComponent.m_Range;
		lightData.position = position.m_Translation;
		inst.m_SceneRenderer->SubmitLight(lightData);
	}

	//render frame
	inst.m_SceneRenderer->Render();
}
void RenderSystem::FixedUpdate(ecs::world& w) {
	Update(w);
}
void RenderSystem::Exit() {
	Instance().Release();
	InstancePtr().reset();
}

std::unique_ptr<RenderSystem::InstanceData>& RenderSystem::InstancePtr() {
	static std::unique_ptr<RenderSystem::InstanceData> instance{ new RenderSystem::InstanceData{} };
	return instance;
}

RenderSystem::InstanceData& RenderSystem::Instance() {
	return *InstancePtr();
}

RenderSystem RenderSystem::System() {
	return RenderSystem();
}


void RenderSystem::RegisterEditorMesh(Resource::Guid guid, std::shared_ptr<Mesh> mesh) {
	g_EditorMeshCache[guid] = mesh;
}

void RenderSystem::RegisterEditorMaterial(Resource::Guid guid, std::shared_ptr<Material> material) {
	g_EditorMaterialCache[guid] = material;
}

// Shared primitive meshes for instancing
namespace {
	std::shared_ptr<Mesh> g_SharedCubeMesh = nullptr;
	std::shared_ptr<Mesh> g_SharedPlaneMesh = nullptr;
}

std::shared_ptr<Mesh> RenderSystem::GetSharedCubeMesh() {
	if (!g_SharedCubeMesh) {
		g_SharedCubeMesh = std::make_shared<Mesh>(PrimitiveGenerator::CreateCube(1.0f));
		assert(g_SharedCubeMesh && "Cube mesh generation failed");
		assert(!g_SharedCubeMesh->vertices.empty() && "Generated cube has no vertices");
	}
	return g_SharedCubeMesh;
}

std::shared_ptr<Mesh> RenderSystem::GetSharedPlaneMesh() {
	if (!g_SharedPlaneMesh) {
		g_SharedPlaneMesh = std::make_shared<Mesh>(PrimitiveGenerator::CreatePlane(2.0f, 2.0f, 1, 1));
		assert(g_SharedPlaneMesh && "Plane mesh generation failed");
		assert(!g_SharedPlaneMesh->vertices.empty() && "Generated plane has no vertices");
	}
	return g_SharedPlaneMesh;
}

void RenderSystem::LoadBasicShaders() {
	auto& instance = RenderSystem::Instance();
	auto* resourceManager = instance.m_SceneRenderer->GetResourceManager();

	assert(resourceManager && "ResourceManager must be initialized");

	// Load PBR shader for main scene objects (following GraphicsTestDriver pattern)
	s_CubeShader = resourceManager->LoadShader("main_pbr",
		"assets/shaders/main_pbr.vert",
		"assets/shaders/main_pbr.frag");

	if (!s_CubeShader) {
		spdlog::error("Failed to load PBR shader for engine cubes");
		assert(false && "PBR shader is required for cube rendering");
		return;
	}

	// Load primitive shader for debug visualization only
	auto primitiveShader = resourceManager->LoadShader("primitive",
		"assets/shaders/primitive.vert",
		"assets/shaders/primitive.frag");

	if (!primitiveShader) {
		spdlog::warn("Failed to load primitive shader for debug visualization");
	}
	else {
		// Configure debug render pass with primitive shader
		instance.m_SceneRenderer->SetDebugPrimitiveShader(primitiveShader);
		spdlog::info("Debug primitive shader loaded successfully");
	}

	// Load shadow depth shader for shadow mapping
	auto shadowShader = resourceManager->LoadShader("shadow_depth",
		"assets/shaders/shadow_depth.vert",
		"assets/shaders/shadow_depth.frag");

	if (!shadowShader) {
		spdlog::warn("Failed to load shadow depth shader - shadows will be disabled");
	}
	else {
		instance.m_SceneRenderer->SetShadowDepthShader(shadowShader);
		spdlog::info("Shadow depth shader loaded successfully");
	}

	// Load picking shader for object selection
	auto pickingShader = resourceManager->LoadShader("picking",
		"assets/shaders/picking.vert",
		"assets/shaders/picking.frag");

	if (!pickingShader) {
		spdlog::warn("Failed to load picking shader - object picking will be disabled");
	}
	else {
		instance.m_SceneRenderer->SetPickingShader(pickingShader);
		spdlog::info("Picking shader loaded successfully");
	}

	spdlog::info("Engine PBR shader system loaded successfully");
}


void RenderSystem::SetupDebugVisualization() {
	// Set up debug visualization meshes (required for DebugRenderPass)
	auto& instance = RenderSystem::Instance();
	auto* sceneRenderer = instance.m_SceneRenderer.get();

	if (sceneRenderer) {
		// Create debug visualization meshes
		auto lightCube = std::make_shared<Mesh>(PrimitiveGenerator::CreateCube(5.0f));
		auto lightRay = std::make_shared<Mesh>(PrimitiveGenerator::CreateDirectionalRay(3.0f));
		auto wireframeCube = std::make_shared<Mesh>(PrimitiveGenerator::CreateWireframeCube(1.0f));

		// Debug visualization uses primitive shader (already set in LoadBasicShaders)
		sceneRenderer->SetDebugLightCubeMesh(lightCube);
		sceneRenderer->SetDebugDirectionalRayMesh(lightRay);
		sceneRenderer->SetDebugAABBWireframeMesh(wireframeCube);

		spdlog::info("Debug visualization meshes configured");
	}
}

auto load_mesh_lambda = [](const char* data)->std::shared_ptr<Mesh> {
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
};

REGISTER_RESOURCE_TYPE_SHARED_PTR(Mesh,
	load_mesh_lambda, [](std::shared_ptr<Mesh>&) {});

REGISTER_RESOURCE_TYPE_SHARED_PTR(Material,
	[](const char* data)->std::shared_ptr<Material> {
	Resource::MaterialAssetData dat = Resource::load_native_material_from_memory(data);
	Resource::ShaderAssetData shdr_dat = *ResourceRegistry::Instance().Get<Resource::ShaderAssetData>(dat.shader_guid);
	std::shared_ptr<Shader> shdr = RenderSystem::Instance().m_SceneRenderer->GetResourceManager()->LoadShader(shdr_dat.m_Name, shdr_dat.m_VertPath, shdr_dat.m_FragPath);
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