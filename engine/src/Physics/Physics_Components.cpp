#include "Physics/Physics_System.h"
#include "Physics/Physics_Components.h"

void Rigidbody::AddForce(const glm::vec3& force) {
    if (bodyID.IsInvalid()) { return; }
    PhysicsSystem::Instance().GetBodyInterface().AddForce(bodyID, PhysicsUtils::ToJolt(force));
}

void Rigidbody::AddImpulse(const glm::vec3& force) {
    if (bodyID.IsInvalid()) { return; }
    PhysicsSystem::Instance().GetBodyInterface().AddImpulse(bodyID, PhysicsUtils::ToJolt(force));
}

void Rigidbody::AddTorque(const glm::vec3& torque) {
    if (bodyID.IsInvalid()) { return; }
    PhysicsSystem::Instance().GetBodyInterface().AddTorque(bodyID, PhysicsUtils::ToJolt(torque));
}
void Rigidbody::AddAngularImpulse(const glm::vec3& AngularImpulse) {
    if (bodyID.IsInvalid()) { return; }
    PhysicsSystem::Instance().GetBodyInterface().AddAngularImpulse(bodyID, PhysicsUtils::ToJolt(AngularImpulse));
}

void Rigidbody::AddLinearVelocity(const glm::vec3& velocity) {
    if (bodyID.IsInvalid()) return;
    PhysicsSystem::Instance().GetBodyInterface().AddLinearVelocity(bodyID, PhysicsUtils::ToJolt(velocity));
}

void Rigidbody::SetLinearVelocity(const glm::vec3& vel) {
    if (bodyID.IsInvalid()) return;
    PhysicsSystem::Instance().GetBodyInterface().SetLinearVelocity(bodyID, PhysicsUtils::ToJolt(vel));
}

void Rigidbody::MoveKinematic(const glm::vec3& targetPos, const glm::vec3& targetRot, const float duration) {
    if (bodyID.IsInvalid()) return;
    PhysicsSystem::Instance().GetBodyInterface().MoveKinematic(bodyID, PhysicsUtils::ToJolt(targetPos), PhysicsUtils::ToQuatJolt(targetRot), duration);
}

void Rigidbody::SetAngularVelocity(const glm::vec3& angVel) {
    if (bodyID.IsInvalid()) return;
    PhysicsSystem::Instance().GetBodyInterface().SetAngularVelocity(bodyID, PhysicsUtils::ToJolt(angVel));
}