#ifndef MANAGED_GAMEOBJECTS_HPP
#define MANAGED_GAMEOBJECTS_HPP
#include <cstdint>

typedef struct _MonoString MonoString;
typedef std::int32_t mono_bool;
class ManagedGameObjects
{
public:
	static void SetVisible(uint64_t handle, mono_bool val);
	static bool GetVisible(uint64_t handle);
	static uint64_t FindByName(MonoString* name);
	static void DeleteGameObject(uint64_t handle);

};


#endif //! MANAGED_GAMEOBJECTS_HPP