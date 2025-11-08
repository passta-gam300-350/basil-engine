#include "Bindings/ManagedPhysics.hpp"
#include "ecs/ecs.h"
#include "Physics/Physics_Components.h"


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
















