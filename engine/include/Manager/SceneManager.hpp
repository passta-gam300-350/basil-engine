#ifndef SCENEMANAGER_HPP
#define SCENEMANAGER_HPP
#include "ecs/fwd.h"

class SceneManager
{

public:
	static void init();
	static void update();
	static void exit();
};

#endif //!SCENEMANAGER_HPP