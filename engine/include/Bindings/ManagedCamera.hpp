/******************************************************************************/
/*!
\file   ManagedCamera.hpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the declaration for the ManagedCamera class, which
is responsible for managing camera-related functionalities in the managed environment.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/


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
