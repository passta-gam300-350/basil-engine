#include "Ecs/ecs.h"
#ifndef ManagedTransform_HPP
#define ManagedTransform_HPP

class ManagedTransform
{
public:
	static void SetPosition(uint64_t handle, float x, float y, float z);
	static void SetRotation(uint64_t handle, float pitch, float yaw, float roll);
	static void SetScale(uint64_t handle, float x, float y, float z);

	static void GetPosition(uint64_t handle, float* x, float* y, float* z);
	static void GetRotation(uint64_t handle, float* pitch, float* yaw, float* roll);
	static void GetScale(uint64_t handle, float* x, float* y, float* z);

};
#endif // ManagedTransform_HPP