#ifndef MANAGED_GAMEOBJECTS_HPP
#define MANAGED_GAMEOBJECTS_HPP
#include <cstdint>

typedef struct _MonoString MonoString;

class ManagedGameObjects
{
public:
	static uint64_t FindByName(MonoString* name);
	static void DeleteGameObject(uint64_t handle);
};


#endif //! MANAGED_GAMEOBJECTS_HPP