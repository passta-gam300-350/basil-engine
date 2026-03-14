#ifndef ManagedWorldUI_HPP
#define ManagedWorldUI_HPP
#include <cstdint>

class ManagedWorldUI
{
	public:
		static void SetScale(uint64_t handle, float x, float y);
		static void GetScale(uint64_t handle, float* px, float* py);

		static void SetVisibility(uint64_t handle, bool visible);
		static bool GetVisibility(uint64_t handle);
	
};

#endif
