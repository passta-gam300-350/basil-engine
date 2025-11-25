/******************************************************************************/
/*!
\file   ManagedPhysics.cpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the implementation for the ManagedPhysics class, which
is responsible for managing and getting various physics-related functionalities in
the managed environment.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#include "Bindings/ManagedPhysics.hpp"

#include "Engine.hpp"
#include "ecs/ecs.h"
#include "Physics/Physics_Components.h"
#include "Physics/Physics_System.h"
#include <glm/glm.hpp>


int ManagedPhysics::GetMotionType(uint64_t handle)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();

	return static_cast<int>(phy.motionType);

	
}

void ManagedPhysics::SetMotionType(uint64_t handle, int motionType)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	phy.motionType = static_cast<RigidBodyComponent::MotionType>(motionType);
}

void ManagedPhysics::SetAngularDrag(uint64_t handle, float angularDrag)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();

	phy.angularDrag = angularDrag;
}

float ManagedPhysics::GetAngularDrag(uint64_t handle)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();

	return phy.angularDrag;
}


void ManagedPhysics::SetAngularVelocity(uint64_t handle, float x, float y, float z)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	glm::vec3 velocity = { x,y,z };
	phy.SetAngularVelocity(velocity);
}

void ManagedPhysics::GetAngularVelocity(uint64_t handle, float* xp, float* yp, float* zp)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();

	glm::vec3 const& velocity = phy.GetAngularVelocity();

	*xp = velocity.x;
	*yp = velocity.y;
	*zp = velocity.z;
}

void ManagedPhysics::SetCenterOfMass(uint64_t handle, float x, float y, float z)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	glm::vec3 com = { x,y,z };
	phy.centerOfMass = com;
}

void ManagedPhysics::GetCenterOfMass(uint64_t handle, float* xp, float* yp, float* zp)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();

	glm::vec3 const& com = phy.centerOfMass;

	*xp = com.x;
	*yp = com.y;
	*zp = com.z;
}

void ManagedPhysics::SetDrag(uint64_t handle, float drag)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();

	phy.drag = drag;
}
float ManagedPhysics::GetDrag(uint64_t handle)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();

	return phy.drag;
}


float ManagedPhysics::GetFriction(uint64_t handle)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	return phy.friction;
}

void ManagedPhysics::SetFriction(uint64_t handle, float friction)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();

	phy.friction = friction;
}

void ManagedPhysics::SetGravityFactor(uint64_t handle, float factor)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();

	phy.gravityFactor = factor;
	
}

float ManagedPhysics::GetGravityFactor(uint64_t handle)
{

	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();

	return phy.gravityFactor;

}

void ManagedPhysics::SetMass(uint64_t handle, float mass)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();

	phy.mass = mass;
}

float ManagedPhysics::GetMass(uint64_t handle)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	return phy.mass;
}

void ManagedPhysics::SetIsKinematic(uint64_t handle, bool isKinematic)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	phy.isKinematic = isKinematic;
}
bool ManagedPhysics::GetIsKinematic(uint64_t handle)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	return phy.isKinematic;
}

void ManagedPhysics::SetUseGravity(uint64_t handle, bool useGravity)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();

	phy.useGravity = useGravity;
}

bool ManagedPhysics::GetUseGravity(uint64_t handle)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	return  phy.useGravity;
}

void ManagedPhysics::SetLinearDamping(uint64_t handle, float damping)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();

	phy.linearDamping = damping;
}

float ManagedPhysics::GetLinearDamping(uint64_t handle)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	return phy.linearDamping;
}

void ManagedPhysics::SetLinearVelocity(uint64_t handle, float x, float y, float z)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();

	glm::vec3 linearVelocity = { x,y,z };

	phy.SetLinearVelocity(linearVelocity);
}

void ManagedPhysics::GetLinearVelocity(uint64_t handle, float* xp, float* yp, float* zp)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();

	glm::vec3 const& vel = phy.GetLinearVelocity();
	*xp = vel.x;
	*yp = vel.y;
	*zp = vel.z;
	

}

void ManagedPhysics::AddForceInternal(uint64_t handle, float x, float y, float z)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();

	glm::vec3 force{ x,y,z };
	phy.AddForce(force);
}

void ManagedPhysics::AddImpulseInternal(uint64_t handle, float x, float y, float z)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();

	glm::vec3 force{ x,y,z };
	phy.AddImpulse(force);
}


void ManagedPhysics::SetFreezeX(uint64_t handle, bool freeze)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	phy.freezePositionX = freeze;
	
}
bool ManagedPhysics::GetFreezeX(uint64_t handle)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	return phy.freezePositionX;
}

bool ManagedPhysics::GetFreezeY(uint64_t handle)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	return phy.freezePositionY;
	
}

void ManagedPhysics::SetFreezeY(uint64_t handle, bool freeze)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	phy.freezePositionY = freeze;
}

void ManagedPhysics::SetFreezeZ(uint64_t handle, bool freeze)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	phy.freezePositionZ = freeze;
}

bool ManagedPhysics::GetFreezeZ(uint64_t handle)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	return phy.freezePositionZ;
}







void ManagedPhysics::SetFreezeRotationX(uint64_t handle, bool freeze)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	phy.freezeRotationX = freeze;
}
bool ManagedPhysics::GetFreezeRotationX(uint64_t handle)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	return phy.freezeRotationX;
}
void ManagedPhysics::SetFreezeRotationY(uint64_t handle, bool freeze)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	phy.freezeRotationY = freeze;
}
bool ManagedPhysics::GetFreezeRotationY(uint64_t handle)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	return phy.freezeRotationY;
}
void ManagedPhysics::SetFreezeRotationZ(uint64_t handle, bool freeze)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	phy.freezeRotationZ = freeze;
}
bool ManagedPhysics::GetFreezeRotationZ(uint64_t handle)
{
	ecs::entity entity{ handle };
	RigidBodyComponent& phy = entity.get<RigidBodyComponent>();
	return phy.freezeRotationZ;
}






void ManagedPhysics::MovePosition(uint64_t handle, float x, float y, float z)
{
	ecs::entity entity{ handle };
	TransformComponent& transform = entity.get<TransformComponent>();
	glm::vec3 pos = { x,y,z };
	glm::vec3 current_rot = transform.m_Rotation;

	RigidBodyComponent& body = entity.get<RigidBodyComponent>();


	body.MoveKinematic(pos, current_rot, static_cast<float>(Engine::Instance().GetDeltaTime()));
}

bool ManagedPhysics::Raycast(float ox, float oy, float oz,
	float dx, float dy, float dz,
	float maxDistance, bool ignoreTriggers,
	uint64_t* entity,
	float* p_x, float* p_y, float* p_z,
	float* n_x, float* n_y, float* n_z,
	float* distance,
	uint8_t* isTrigger)
{
	auto hit = PhysicsSystem::Instance().Raycast(glm::vec3{ ox, oy, oz }, glm::vec3{ dx, dy, dz }, maxDistance, ignoreTriggers);

	if (hit.hasHit)
	{
		if (entity) *entity = hit.entity ? hit.entity.get_uuid() : 0;
		if (p_x) *p_x = hit.hitPoint.x;
		if (p_y) *p_y = hit.hitPoint.y;
		if (p_z) *p_z = hit.hitPoint.z;
		if (n_x) *n_x = hit.hitNormal.x;
		if (n_y) *n_y = hit.hitNormal.y;
		if (n_z) *n_z = hit.hitNormal.z;
		if (distance) *distance = hit.distance;
		if (isTrigger) *isTrigger = hit.isTrigger ? 1 : 0;
	}
	else
	{
		if (entity) *entity = 0;
		if (p_x) *p_x = 0.f;
		if (p_y) *p_y = 0.f;
		if (p_z) *p_z = 0.f;
		if (n_x) *n_x = 0.f;
		if (n_y) *n_y = 0.f;
		if (n_z) *n_z = 0.f;
		if (distance) *distance = 0.f;
		if (isTrigger) *isTrigger = 0;
	}

	return hit.hasHit;
}










