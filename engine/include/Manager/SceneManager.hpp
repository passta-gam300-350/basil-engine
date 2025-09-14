#ifndef SCENEMANAGER_HPP
#define SCENEMANAGER_HPP
#include "ecs/fwd.h"

class Scene;

class SceneManager
{
private:
	static SceneManager* _instance;

	std::unordered_map<uint32_t, Scene> scenes;

	Scene* currentScene{ nullptr };

public:

	void init();
	void update();
	void exit();

	static SceneManager& GetInstance();


	void AddScene(Scene const&);
	void RemoveScene(std::string const&);

	Scene CreateEmpty(std::string const& name);

};

#endif //!SCENEMANAGER_HPP