#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "Engine.hpp"
#include <glm/glm.hpp>
#include <Render/Render.h>
#include <Manager/ResourceSystem.hpp>
#include <components/transform.h>
#include <filesystem>

int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//Engine::GenerateDefaultConfig();
	//_CrtSetBreakAlloc(4657);
	Engine::Init("Default.yaml");
	ecs::world w = Engine::GetWorld();

	w.add_entity().add<glm::mat4>(glm::vec4{323.231, 231.3, 545.43, 0.433}, glm::vec4{ 323.231, 231.3, 545.43, 0.433 }, glm::vec4{ 323.231, 231.3, 545.43, 0.433 }, glm::vec4{ 0, 0, 0, 0 });

	w.SaveYAML("testreflectserial.yaml");

	//Engine::Init("Default.yaml");
	/*
	auto e1 = Engine::GetWorld().add_entity();
	auto& m = e1.add<MeshRendererComponent>();
	auto& t = e1.add<TransformComponent>();
	auto& v = e1.add<VisibilityComponent>();

	auto c1 = Engine::GetWorld().add_entity();
	auto& pos = c1.add<PositionComponent>();
	auto& cam = c1.add<CameraComponent>();

	pos.m_WorldPos = { 0.25f, 0.75f, 0.25f };

	cam.m_Type = CameraComponent::CameraType::PERSPECTIVE;
	cam.m_Fov = 45.f;
	cam.m_AspectRatio = 16.0f / 9.0f;
	cam.m_Near = 0.1f;
	cam.m_Far = 1000.0f;
	cam.m_IsActive = true;
	//cam.m_Up = { 0.f, 1.f, 0.f };
	//cam.m_Right = { 1.f, 0.f, 0.f };

	Resource::Guid gud;
	gud.m_high = 0;
	gud.m_low = 101;
	m.m_MeshGuid = gud;
	v.m_IsVisible = true;
	t.m_trans = glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(0.f,0.f,0.f)), glm::vec3(1.f));

	std::uint64_t sz{ std::filesystem::file_size("test/101.mesh") };

	ResourceSystem::Instance().m_FileEntries[gud].m_Path = "test/101.mesh";
	ResourceSystem::Instance().m_FileEntries[gud].m_Offset = 0;
	ResourceSystem::Instance().m_FileEntries[gud].m_Size = sz;
	ResourceSystem::Instance().m_FileEntries[gud].m_Guid = gud;

	ResourceSystem::Instance().m_MappedIO.emplace("test/101.mesh",MemoryMappedFile(L"test/101.mesh"));

	gud.m_low = 1001;

	m.m_MaterialGuid = gud;

	sz = std::filesystem::file_size("test/1001.material");

	ResourceSystem::Instance().m_FileEntries[gud].m_Path = "test/1001.material";
	ResourceSystem::Instance().m_FileEntries[gud].m_Offset = 0;
	ResourceSystem::Instance().m_FileEntries[gud].m_Size = sz;
	ResourceSystem::Instance().m_FileEntries[gud].m_Guid = gud;

	ResourceSystem::Instance().m_MappedIO.emplace("test/1001.material", MemoryMappedFile(L"test/1001.material"));

	gud.m_low = 10001;

	sz = std::filesystem::file_size("test/10001.shader");

	ResourceSystem::Instance().m_FileEntries[gud].m_Path = "test/10001.shader";
	ResourceSystem::Instance().m_FileEntries[gud].m_Offset = 0;
	ResourceSystem::Instance().m_FileEntries[gud].m_Size = sz;
	ResourceSystem::Instance().m_FileEntries[gud].m_Guid = gud;

	ResourceSystem::Instance().m_MappedIO.emplace("test/10001.shader", MemoryMappedFile(L"test/10001.shader"));

	gud.m_low = 10;

	sz = std::filesystem::file_size("test/10.dds");

	ResourceSystem::Instance().m_FileEntries[gud].m_Path = "test/10.dds";
	ResourceSystem::Instance().m_FileEntries[gud].m_Offset = 0;
	ResourceSystem::Instance().m_FileEntries[gud].m_Size = sz;
	ResourceSystem::Instance().m_FileEntries[gud].m_Guid = gud;

	ResourceSystem::Instance().m_MappedIO.emplace("test/10.dds", MemoryMappedFile(L"test/10.dds"));

	Engine::Update();
	//Engine::GetWorld().LoadYAML("testscene.yaml");
	/*
	auto e = Engine::GetWorld().filter_entities<glm::vec4>();
	auto it = e.begin();
	auto ename = it->name();
	glm::vec4 test = it->get<glm::vec4>();
	*/
	Engine::Exit();
	return 0;
}