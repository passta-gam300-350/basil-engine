#ifndef MANAGEDCAMERA_HPP
#define	MANAGEDCAMERA_HPP
#include <cstdint>

class ManagedCamera
{
public:
	static int GetCameraType(uint64_t handle);
	static void SetCameraType(uint64_t handle, int type);

	static float getFoV(uint64_t handle);
	static void setFoV(uint64_t handle, float fov);

	static float getAspectRatio(uint64_t handle);
	static void setAspectRatio(uint64_t handle, float aspectRatio);

	static float getNear(uint64_t handle);
	static void setNear(uint64_t handle, float nearClip);

	static float getFar(uint64_t handle);
	static void setFar(uint64_t handle, float farClip);





};

#endif // MANAGEDCAMERA_HPP
