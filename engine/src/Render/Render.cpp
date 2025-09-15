#include <Core/Window.h>
#include "Render/Render.h"
#include "Ecs/ecs.h"
#include "Engine.hpp"
#include "components/transform.h"
#include "Manager/ResourceSystem.hpp"

#include <tinyddsloader.h>

std::unique_ptr<RenderSystem> RenderSystem::s_Instance{};

void RenderSystem::Init() {
	m_SceneRenderer = std::make_unique<SceneRenderer>(Engine::GetWindowInstance().GetNativeWindow());
	m_ResourceManager = m_SceneRenderer->GetResourceManager();
	m_Camera = std::make_unique<Camera>(CameraType::Perspective);
	m_Camera->SetPerspective(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
	m_Camera->SetPosition(glm::vec3(0.0f, 2.0f, 8.0f));
	m_Camera->SetRotation(glm::vec3(-10.0f, 0.0f, 0.0f));
}


void RenderSystem::Update(ecs::world& world) {
	//begin frame
	m_SceneRenderer->ClearFrame();

	auto world_cameras = world.filter_entities<CameraComponent, TransformComponent, PositionComponent>();
	//no camera management yet. just pick the first entity
	if (world_cameras) {
		auto& frameData = m_SceneRenderer->GetFrameData();
		auto [camera, camera_pos] {(*world_cameras.begin()).get<CameraComponent, PositionComponent>()};
		m_Camera->SetPosition(camera_pos.m_WorldPos);
		if (camera.m_Type == CameraComponent::CameraType::PERSPECTIVE) {
			m_Camera->SetPerspective(camera.m_Fov, camera.m_AspectRatio, camera.m_Near, camera.m_Far);
		}
		else {
			//m_Camera->SetPerspective(camera.m_Fov, camera.m_AspectRatio, camera.m_Near, camera.m_Far);
		}
		frameData.viewMatrix = m_Camera->GetViewMatrix();
		frameData.projectionMatrix = m_Camera->GetProjectionMatrix();
		frameData.cameraPosition = m_Camera->GetPosition();
	}
	auto sceneObjects = world.filter_entities<MeshRendererComponent, TransformComponent, VisibilityComponent>();
	auto sceneLights = world.filter_entities<LightComponent, PositionComponent>();
	m_SceneObjects.reserve(sceneObjects.entities.size_hint());
	m_SceneObjects.reserve(sceneLights.entities.size_hint());
	for (auto obj : sceneObjects) {
		auto [mesh, transform, visible] {obj.get<MeshRendererComponent, TransformComponent, VisibilityComponent>()};
		RenderableData renderData;
		renderData.mesh = ResourceSystem::Get<ResourceTypeMesh>(mesh.m_MeshGuid)->m_ptr;
		//renderData.material = ResourceSystem::Get<Material>(mesh.m_MaterialGuid);
		renderData.transform = transform.m_trans;
		renderData.visible = visible.m_IsVisible;
		renderData.renderLayer = 1;
		m_SceneObjects.emplace_back(renderData);
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
		m_SceneLights.emplace_back(lightData);
	}

	//very dangerous hack. clears all renderables. TODO: replace this when there is a dedicated vector for dynamic objects
	*reinterpret_cast<std::vector<RenderableData>*>(m_SceneRenderer.get()) = std::move(m_SceneObjects);
	reinterpret_cast<std::vector<SubmittedLightData>*>(m_SceneRenderer.get())[1] = std::move(m_SceneLights);

	//render frame
	m_SceneRenderer->Render();
}
void RenderSystem::FixedUpdate(ecs::world&) {
}
void RenderSystem::Exit() {
	m_Renderer.reset();
	m_SceneRenderer.reset();
	m_Camera.reset();
	s_Instance.reset();
}


void RenderSystem::NewInstance() {
	s_Instance.reset(new RenderSystem{});
}

ResourceTypeMesh::ResourceTypeMesh(Resource::MeshResource const& mr)
{
	m_ptr = std::make_shared<Mesh>(std::vector<Vertex>{}, mr.m_indices, std::vector<Texture>{});
	for (std::uint64_t i{}; i < mr.m_positions.size(); i++) {
		Vertex v{};
		v.Position = mr.m_positions[i];
		m_ptr->vertices.emplace_back(v);
	}
}


RenderSystem& RenderSystem::Instance() {
	return *s_Instance;
}