#ifndef EDITORMAIN_HPP
#define EDITORMAIN_HPP

#include "imgui.h"
#include "Screens/Screen.hpp"

class EditorMain : public Screen
{
public:
	bool showAboutModal = false;




	EditorMain(GLFWwindow* window);
	void init() override;
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



	void Render_Console();



	void Render_Scene();
	void Render_Game();

private:
	// Entity management
	void CreateDefaultEntity();
	void CreateCubeEntity();
	void CreateLightEntity();
	void CreateCameraEntity();
};
#endif // EDITORMAIN_HPP