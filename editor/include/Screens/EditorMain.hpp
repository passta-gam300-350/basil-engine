#ifndef EDITORMAIN_HPP
#define EDITORMAIN_HPP

#include "imgui.h"
#include "Screens/Screen.hpp"
#include "Camera/EditorCamera.hpp"
#include "ecs/fwd.h"
#include "Manager/AssetManager.hpp"
#include <memory>

#include "Service/FileService.hpp"

class EditorMain : public Screen
{
	FileService fileService;
public:
	bool showAboutModal = false;
	bool showInspector = true;
	bool showSceneExplorer = true;
	bool showProfiler = false;
	bool showConsole = true;





	EditorMain(GLFWwindow* window);
	void init() override;
	void update() override;
	void render() override;
	void cleanup() override;
	void Show() override;
	void Close() override;
	virtual bool isWindowClosed() override;
	bool Activate() override;





	~EditorMain() override = default;



	void Setup_Dockspace(unsigned id);

	void Render_MenuBar();

	void Render_AboutUI();


	void Render_SceneExplorer();

	void Render_Profiler();

	void Render_Inspector();

	void Render_Console();

	void Render_AssetBrowser();

	void Render_Scene();
	void Render_Game();
	void Render_CameraControls();


	void Render_Transform_Group_Component(ecs::entity entity_handle);

	void Render_Lighting_Group_Component(ecs::entity entity_handle);

	void Render_Camera_Group_Component(ecs::entity entity_handle);
	void Render_Mesh_Component(ecs::entity entity_handle);

private:
	// Entity management
	void CreateDefaultEntity();
	void CreatePlaneEntity();
	void CreateLightEntity();
	void CreateCameraEntity();
	void CreateDemoScene();

	// Demo scene creation utilities
	void CreateCube(const glm::vec3& position = glm::vec3(0.0f),
	               const glm::vec3& scale = glm::vec3(1.0f),
	               const glm::vec3& color = glm::vec3(1.0f, 0.0f, 0.0f));
	void CreateCubeGrid(int gridSize = 3, float spacing = 3.0f);

	void CreatePhysicsDemoScene();
	void CreatePhysicsCube();

	// Editor Camera
	std::unique_ptr<EditorCamera> m_EditorCamera;

	// Asset Manager
	std::unique_ptr<AssetManager> m_AssetManager;

	// Play mode state
	bool m_IsPlayMode = false;

	// Viewport size tracking for aspect ratio
	float m_ViewportWidth = 1280.0f;
	float m_ViewportHeight = 720.0f;

	// Entity selection management
	uint32_t m_SelectedEntityID = 0;         // Currently selected entity's object ID (0 = none)
	bool m_ShowSelectionInfo = true;         // Show selection info in inspector

	// Debug rendering controls
	bool m_ShowAABBs = false;

	// Viewport picking implementation
	void HandleViewportPicking();
	void PerformEntityPicking(float mouseX, float mouseY, float viewportWidth, float viewportHeight);
	void SelectEntity(uint32_t objectID);
	void ClearEntitySelection();

	// Debug visualization control
	void SetDebugVisualization(bool showAABBs);


	void SaveScene(const char* path);
	void LoadScene(const char* name);
};
#endif // EDITORMAIN_HPP