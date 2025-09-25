#include <Core/Window.h>
#include "Render/Render.h"
#include "Engine.hpp"
#include "components/transform.h"
#include "Manager/ResourceSystem.hpp"

#include "native/loader.h"
#include "native/texture.h"

#include <tinyddsloader.h>
#include "Input/InputManager.h"

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
		RenderableData renderData;
		//renderData.mesh = ResourceSystem::Get<ResourceTypeMesh>(mesh.m_MeshGuid)->m_ptr;
		//renderData.material = ResourceSystem::Get<Material>(mesh.m_MaterialGuid);
		renderData.mesh = *ResourceRegistry::Instance().Get<std::shared_ptr<Mesh>>(mesh.m_MeshGuid);
		renderData.material = *ResourceRegistry::Instance().Get<std::shared_ptr<Material>>(mesh.m_MaterialGuid);
		renderData.transform = transform.m_trans;
		renderData.visible = visible.m_IsVisible;
		renderData.renderLayer = 1;
		inst.m_SceneRenderer->SubmitRenderable(renderData);
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

namespace {
	static_assert(std::is_base_of_v<ecs::SystemBase, RenderSystem> && "RenderSystem" && "Type is not derived"); static_assert(!std::is_abstract_v<RenderSystem> && "RenderSystem" && "Type should not be abstract"); static_assert(is_specialization_of_v<ecs::ReadSet<MeshRendererComponent, TransformComponent, VisibilityComponent, LightComponent, CameraComponent>, ecs::QuerySetBasic> && "(ecs::ReadSet<MeshRendererComponent, TransformComponent, VisibilityComponent, LightComponent, CameraComponent>)" && "Type should a query set"); static_assert(is_specialization_of_v<ecs::WriteSet<>, ecs::QuerySetBasic> && "(ecs::WriteSet<>)" && "Type should a query set"); static auto Render_SYSTEM_FACTORY = []()->ecs::SystemBase* { return new RenderSystem{}; }; static auto Render_SYSTEM_CONFIG_GENERATOR = []()->YAML::Node { return YAML::Node{}; }; 
	auto Render_SYSTEM_REG = [&] { ecs::SystemRegistry::Instance().RegisterSystem({ "Render", std::uint64_t(&Render_SYSTEM_FACTORY), ecs::ReadSet<MeshRendererComponent, TransformComponent, VisibilityComponent, LightComponent, CameraComponent> ::GetSet(), ecs::WriteSet<> ::GetSet(), 60.f, false, Render_SYSTEM_FACTORY }); return 0; }();
}