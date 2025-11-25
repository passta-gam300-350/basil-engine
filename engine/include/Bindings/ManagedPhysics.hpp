/******************************************************************************/
/*!
\file   ManagedPhysics.hpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the declaration for the ManagedPhysics class, which
is responsible for managing and getting various physics-related functionalities in
the managed environment.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef MANAGEDPHYSICS_HPP
#define MANAGEDPHYSICS_HPP
#include <cstdint>
#include <cstddef>

class ManagedPhysics
{
public:
	static void SetMotionType(uint64_t handle, int motionType);
	static int GetMotionType(uint64_t handle);

	static void SetMass(uint64_t handle, float mass);
	static float GetMass(uint64_t handle);

	static void SetDrag(uint64_t handle, float drag);
	static float GetDrag(uint64_t handle);

	static void SetFriction(uint64_t handle, float friction);
	static float GetFriction(uint64_t handle);

	static void SetAngularDrag(uint64_t handle, float angularDrag);
	static float GetAngularDrag(uint64_t handle);

	static void SetGravityFactor(uint64_t handle, float factor);
	static float GetGravityFactor(uint64_t handle);

	static void SetUseGravity(uint64_t handle, bool useGravity);
	static bool GetUseGravity(uint64_t handle);

	static void SetIsKinematic(uint64_t handle, bool isKinematic);
	static bool GetIsKinematic(uint64_t handle);


	static void SetFreezeX(uint64_t handle, bool freeze);
	static bool GetFreezeX(uint64_t handle);

	static void SetFreezeY(uint64_t handle, bool freeze);
	static bool GetFreezeY(uint64_t handle);

	static void SetFreezeZ(uint64_t handle, bool freeze);
	static bool GetFreezeZ(uint64_t handle);


	static void SetFreezeRotationX(uint64_t handle, bool freeze);
	static bool GetFreezeRotationX(uint64_t handle);

	static void SetFreezeRotationY(uint64_t handle, bool freeze);
	static bool GetFreezeRotationY(uint64_t handle);

	static void SetFreezeRotationZ(uint64_t handle, bool freeze);
	static bool GetFreezeRotationZ(uint64_t handle);

	static void SetLinearDamping(uint64_t handle, float damping);
	static float GetLinearDamping(uint64_t handle);

	static void SetCenterOfMass(uint64_t handle, float x, float y, float z);
	static void GetCenterOfMass(uint64_t handle, float* xp, float* yp, float* zp);

	static void SetLinearVelocity(uint64_t handle, float x, float y, float z);
	static void GetLinearVelocity(uint64_t handle, float* xp, float* yp, float* zp);

	static void SetAngularVelocity(uint64_t handle, float x, float y, float z);
	static void GetAngularVelocity(uint64_t handle, float* xp, float* yp, float* zp);

	static void AddForceInternal(uint64_t handle, float x, float y, float z);
	static void AddImpulseInternal(uint64_t handle, float x, float y, float z);

	static void MovePosition(uint64_t handle, float x, float y, float z);

	// Raycast returning hit info
	static bool Raycast(float ox, float oy, float oz,
		float dx, float dy, float dz,
		float maxDistance, bool ignoreTriggers,
		uint64_t* entity,
		float* p_x, float* p_y, float* p_z,
		float* n_x, float* n_y, float* n_z,
		float* distance,
		uint8_t* isTrigger);
};

#endif
