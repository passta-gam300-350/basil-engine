#include <Core/Window.h>
#include "Render/Render.h"
#include "Engine.hpp"
#include "components/transform.h"
#include "Manager/ResourceSystem.hpp"

#include "native/loader.h"
#include "native/texture.h"

#include <tinyddsloader.h>
#include "Input/InputManager.h"
#include <Resources/PrimitiveGenerator.h>
#include <Resources/Material.h>
#include <spdlog/spdlog.h>

// Editor resource caches at file scope
namespace {
	std::unordered_map<Resource::Guid, std::shared_ptr<Mesh>> g_EditorMeshCache;
	std::unordered_map<Resource::Guid, std::shared_ptr<Material>> g_EditorMaterialCache;
}

// Static shader storage for cube system
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

	// Automatically create debug cubes when render system initializes
	InitializeDebugCubes();

	spdlog::info("RenderSystem initialized with automatic cube generation");
}

void RenderSystem::Update(ecs::world& world) {
	InstanceData& inst{ Instance() };
	//begin frame
	inst.m_SceneRenderer->ClearFrame();

	auto world_cameras = world.filter_entities<CameraComponent, PositionComponent>();
	auto& frameData = inst.m_SceneRenderer->GetFrameData();
	//no camera management yet. just pick the first entity
	if (world_cameras) {
		auto [camera, camera_pos] {(*world_cameras.begin()).get<CameraComponent, PositionComponent>()};
		inst.m_Camera->SetPosition(camera_pos.m_WorldPos);
		//for testing only. remove this in the future
		if (InputManager::Get_Instance()->Is_KeyPressed(GLFW_KEY_W)) 
		{
			double x{};
			double y{1};
			inst.m_Camera->SetMouseSensitivity(0.1f);
			inst.m_Camera->ProcessMouseMovement(x, y);
		}
		if (InputManager::Get_Instance()->Is_KeyPressed(GLFW_KEY_A))
		{
			double x{1};
			double y{};
			inst.m_Camera->SetMouseSensitivity(0.1f);
			inst.m_Camera->ProcessMouseMovement(x, y);
		}
		if (InputManager::Get_Instance()->Is_KeyPressed(GLFW_KEY_D))
		{
			double x{-1};
			double y{ };
			inst.m_Camera->SetMouseSensitivity(0.1f);
			inst.m_Camera->ProcessMouseMovement(x, y);
		}
		if (InputManager::Get_Instance()->Is_KeyPressed(GLFW_KEY_S))
		{
			double x{  };
			double y{ -1};
			inst.m_Camera->SetMouseSensitivity(0.1f);
			inst.m_Camera->ProcessMouseMovement(x, y);
		}
		if (InputManager::Get_Instance()->Is_KeyPressed(GLFW_KEY_RIGHT))
		{
			inst.m_Camera->ProcessKeyboard(CameraMovement::RIGHT, 0.166f);
		}
		if (InputManager::Get_Instance()->Is_KeyPressed(GLFW_KEY_UP))
		{
			inst.m_Camera->ProcessKeyboard(CameraMovement::UP, 0.166f);
		}
		if (InputManager::Get_Instance()->Is_KeyPressed(GLFW_KEY_DOWN))
		{
			inst.m_Camera->ProcessKeyboard(CameraMovement::DOWN, 0.166f);
		}
		if (InputManager::Get_Instance()->Is_KeyPressed(GLFW_KEY_LEFT))
		{
			inst.m_Camera->ProcessKeyboard(CameraMovement::LEFT, 0.166f);
		}

		if (camera.m_Type == CameraComponent::CameraType::PERSPECTIVE) {
			inst.m_Camera->SetPerspective(camera.m_Fov, camera.m_AspectRatio, camera.m_Near, camera.m_Far);
		}
		else {
			//m_Camera->SetPerspective(camera.m_Fov, camera.m_AspectRatio, camera.m_Near, camera.m_Far);
		}
	}
	frameData.viewMatrix = inst.m_Camera->GetViewMatrix();
	frameData.projectionMatrix = inst.m_Camera->GetProjectionMatrix();
	frameData.cameraPosition = inst.m_Camera->GetPosition();

	auto sceneObjects = world.filter_entities<MeshRendererComponent, TransformComponent, VisibilityComponent>();
	auto sceneLights = world.filter_entities<LightComponent, PositionComponent>();
	for (auto obj : sceneObjects) {
		auto [mesh, transform, visible] {obj.get<MeshRendererComponent, TransformComponent, VisibilityComponent>()};

		std::shared_ptr<Mesh> meshResource;
		std::shared_ptr<Material> materialResource;

		// Try editor cache first (for runtime-created assets)
		if (g_EditorMeshCache.contains(mesh.m_MeshGuid)) {
			meshResource = g_EditorMeshCache[mesh.m_MeshGuid];
		} else {
			// Fall back to file-based registry
			auto* meshPtr = ResourceRegistry::Instance().Get<std::shared_ptr<Mesh>>(mesh.m_MeshGuid);
			if (meshPtr) {
				meshResource = *meshPtr;
			}
		}

		if (g_EditorMaterialCache.contains(mesh.m_MaterialGuid)) {
			materialResource = g_EditorMaterialCache[mesh.m_MaterialGuid];
		} else {
			// Fall back to file-based registry
			auto* materialPtr = ResourceRegistry::Instance().Get<std::shared_ptr<Material>>(mesh.m_MaterialGuid);
			if (materialPtr) {
				materialResource = *materialPtr;
			}
		}

		// Only render if we have both mesh and material
		if (meshResource && materialResource) {
			RenderableData renderData;
			renderData.mesh = meshResource;
			renderData.material = materialResource;
			renderData.transform = transform.m_trans;
			renderData.visible = visible.m_IsVisible;
			renderData.renderLayer = 1;
			inst.m_SceneRenderer->SubmitRenderable(renderData);
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

void RenderSystem::LoadBasicShaders() {
	auto& instance = RenderSystem::Instance();
	auto* resourceManager = instance.m_SceneRenderer->GetResourceManager();

	assert(resourceManager && "ResourceManager must be initialized");

	// Load primitive shader (available in bin directory)
	s_CubeShader = resourceManager->LoadShader("engine_primitive",
		"assets/shaders/primitive.vert",
		"assets/shaders/primitive.frag");

	if (!s_CubeShader) {
		spdlog::error("Failed to load basic shader for engine cubes");
		assert(false && "Basic shader is required for cube rendering");
		return;
	}

	spdlog::info("Engine cube shader loaded successfully");
}

void RenderSystem::CreateDebugCube(const glm::vec3& position,
								  const glm::vec3& scale,
								  const glm::vec3& color) {
	assert(scale.x > 0 && scale.y > 0 && scale.z > 0 && "Scale must be positive");

	// Ensure shader is loaded
	if (!s_CubeShader) {
		LoadBasicShaders();
	}

	auto world = Engine::GetWorld();
	auto entity = world.add_entity();

	// Add transform components (same as editor entity creation)
	world.add_component_to_entity<PositionComponent>(entity, position);
	world.add_component_to_entity<TransformComponent>(entity,
		glm::scale(glm::translate(glm::mat4(1.0f), position), scale));
	world.add_component_to_entity<VisibilityComponent>(entity, true);

	// Create cube mesh using PrimitiveGenerator (no models needed)
	auto cubeMesh = std::make_shared<Mesh>(PrimitiveGenerator::CreateCube(1.0f));
	assert(cubeMesh && "Cube mesh generation failed");
	assert(!cubeMesh->vertices.empty() && "Generated cube has no vertices");

	// Create material with color for primitive shader
	auto material = std::make_shared<Material>(s_CubeShader, "EngineCube_" + std::to_string(entity.get_uid()));
	material->SetAlbedoColor(color);  // This sets PBR albedo
	material->SetVec3("u_Color", color);  // This sets the primitive shader's color uniform
	material->SetMetallicValue(0.1f);
	material->SetRoughnessValue(0.8f);

	assert(material && "Material creation failed");

	// Register in editor cache (same as current editor workflow)
	auto meshGuid = Resource::Guid::generate();
	auto materialGuid = Resource::Guid::generate();

	RegisterEditorMesh(meshGuid, cubeMesh);
	RegisterEditorMaterial(materialGuid, material);

	// Add mesh renderer component
	MeshRendererComponent meshRenderer;
	meshRenderer.m_MeshGuid = meshGuid;
	meshRenderer.m_MaterialGuid = materialGuid;

	world.add_component_to_entity<MeshRendererComponent>(entity, meshRenderer);

	spdlog::info("Created engine cube at ({:.1f}, {:.1f}, {:.1f}) with color ({:.2f}, {:.2f}, {:.2f})",
				 position.x, position.y, position.z, color.r, color.g, color.b);
}

void RenderSystem::CreateCubeGrid(int gridSize, float spacing) {
	assert(gridSize > 0 && gridSize <= 10 && "Grid size must be between 1-10");
	assert(spacing > 0.0f && "Spacing must be positive");

	spdlog::info("Creating {}x{} cube grid (engine cubes)", gridSize, gridSize);

	// Same colors as GraphicsTestDriver
	std::vector<glm::vec3> colors = {
		glm::vec3(0.8f, 0.2f, 0.2f),  // Red
		glm::vec3(0.2f, 0.8f, 0.2f),  // Green
		glm::vec3(0.2f, 0.2f, 0.8f),  // Blue
		glm::vec3(1.0f, 0.8f, 0.2f),  // Gold
		glm::vec3(1.0f, 1.0f, 1.0f),  // White
	};

	const float startOffset = -(gridSize - 1) * spacing * 0.5f;

	for (int x = 0; x < gridSize; ++x) {
		for (int z = 0; z < gridSize; ++z) {
			glm::vec3 position(
				startOffset + x * spacing,
				0.0f,  // Ground level
				startOffset + z * spacing
			);

			// Cycle through materials based on position (same as GraphicsTestDriver)
			int colorIndex = (x + z) % colors.size();
			glm::vec3 color = colors[colorIndex];

			CreateDebugCube(position, glm::vec3(1.0f), color);
		}
	}

	spdlog::info("Created {} engine cubes in grid", gridSize * gridSize);
}

void RenderSystem::InitializeDebugCubes() {
	spdlog::info("=== Engine Cube System Initialization ===");

	// Load shaders first
	LoadBasicShaders();

	// Set up debug visualization meshes (required for DebugRenderPass)
	auto& instance = RenderSystem::Instance();
	auto* sceneRenderer = instance.m_SceneRenderer.get();

	if (sceneRenderer && s_CubeShader) {
		// Create debug visualization meshes
		auto lightCube = std::make_shared<Mesh>(PrimitiveGenerator::CreateCube(5.0f));
		auto lightRay = std::make_shared<Mesh>(PrimitiveGenerator::CreateDirectionalRay(3.0f));
		auto wireframeCube = std::make_shared<Mesh>(PrimitiveGenerator::CreateWireframeCube(1.0f));

		// Configure debug meshes in SceneRenderer
		sceneRenderer->SetDebugPrimitiveShader(s_CubeShader);
		sceneRenderer->SetDebugLightCubeMesh(lightCube);
		sceneRenderer->SetDebugDirectionalRayMesh(lightRay);
		sceneRenderer->SetDebugAABBWireframeMesh(wireframeCube);

		spdlog::info("Debug visualization meshes configured");
	}

	// Create cube grid (emulating GraphicsTestDriver::SetupAdvancedScene)
	CreateCubeGrid(3, 3.0f);

	// Add basic lighting (same as GraphicsTestDriver)
	auto world = Engine::GetWorld();

	// Create directional light entity
	auto lightEntity = world.add_entity();
	world.add_component_to_entity<PositionComponent>(lightEntity, glm::vec3(0.0f, 5.0f, 0.0f));

	LightComponent light;
	light.m_Type = Light::Type::Directional;
	light.m_Direction = glm::vec3(0.2f, -0.8f, 0.3f);
	light.m_Color = glm::vec3(1.0f, 0.95f, 0.85f);
	light.m_Intensity = 1.0f;
	light.m_IsEnabled = true;

	world.add_component_to_entity<LightComponent>(lightEntity, light);

	spdlog::info("Engine cube system initialized with {} cubes and lighting", 9);
	spdlog::info("==========================================");
}

auto load_mesh_lambda = [](const char* data)->std::shared_ptr<Mesh> {
	Resource::MeshAssetData dat = Resource::load_native_mesh_from_memory(data);
	std::vector<Texture> textures{};
	for (auto tex_guid : dat.textures) {
		Resource::TextureAssetData& tex = *ResourceRegistry::Instance().Get<Resource::TextureAssetData>(tex_guid);
		const DirectX::Image* img = tex.GetImage(0, 0, 0);
		DirectX::ScratchImage decompressed;
		if (DirectX::IsCompressed(img->format)) {
			auto hr = DirectX::Decompress(*img, DXGI_FORMAT_R8G8B8A8_UNORM, decompressed);
			assert(!FAILED(hr) && "decompress failed");
			img = decompressed.GetImage(0, 0, 0);
		}
		TextureData tdata{};
		size_t texelSize = img->rowPitch * img->height;
		tdata.pixels = new unsigned char[texelSize];
		tdata.width = img->rowPitch;
		tdata.height = img->height;
		memcpy((void*)(tdata.pixels), (const void*)(img->pixels), texelSize);
		Texture texture;
		texture.id = TextureLoader::CreateGPUTexture(tdata);
		texture.type = "texture_diffuse";
		textures.emplace_back(texture);
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