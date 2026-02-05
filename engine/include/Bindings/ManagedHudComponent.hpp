#ifndef ManagedHudComponent_HPP
#define ManagedHudComponent_HPP
#include <cstdint>

class ManagedHudComponent
{
public:

	static void GetPosition(uint64_t handle, float* out_x, float* out_y);
	static void SetPosition(uint64_t handle, float x, float y);

	static bool GetVisibility(uint64_t handle);
	static void SetVisibility(uint64_t handle, bool visible);

};

#endif
