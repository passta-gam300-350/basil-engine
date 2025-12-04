#include "Physics/Physics_System.h"
#include "Physics/Physics_Components.h"

#include "spdlog/spdlog.h"

void RigidBodyComponent::AddForce(const glm::vec3& force) {
    JPH::BodyID bodyID = PhysicsSystem::Instance().GetBodyID(ecs::entity(m_entity));
    if (bodyID.IsInvalid()) { return; }
    PhysicsSystem::Instance().GetBodyInterface().AddForce(bodyID, PhysicsUtils::ToJolt(force));
}

void RigidBodyComponent::AddImpulse(const glm::vec3& force) {
    JPH::BodyID bodyID = PhysicsSystem::Instance().GetBodyID(ecs::entity(m_entity));
    if (bodyID.IsInvalid()) { return; }
    PhysicsSystem::Instance().GetBodyInterface().AddImpulse(bodyID, PhysicsUtils::ToJolt(force));
}

void RigidBodyComponent::AddTorque(const glm::vec3& torque) {
    JPH::BodyID bodyID = PhysicsSystem::Instance().GetBodyID(ecs::entity(m_entity));
    if (bodyID.IsInvalid()) { return; }
    PhysicsSystem::Instance().GetBodyInterface().AddTorque(bodyID, PhysicsUtils::ToJolt(torque));
}
void RigidBodyComponent::AddAngularImpulse(const glm::vec3& AngularImpulse) {
    JPH::BodyID bodyID = PhysicsSystem::Instance().GetBodyID(ecs::entity(m_entity));
    if (bodyID.IsInvalid()) { return; }
    PhysicsSystem::Instance().GetBodyInterface().AddAngularImpulse(bodyID, PhysicsUtils::ToJolt(AngularImpulse));
}

void RigidBodyComponent::AddLinearVelocity(const glm::vec3& velocity) {
    JPH::BodyID bodyID = PhysicsSystem::Instance().GetBodyID(ecs::entity(m_entity));
    if (bodyID.IsInvalid()) return;
    PhysicsSystem::Instance().GetBodyInterface().AddLinearVelocity(bodyID, PhysicsUtils::ToJolt(velocity));
}

void RigidBodyComponent::SetLinearVelocity(const glm::vec3& vel) {
    JPH::BodyID bodyID = PhysicsSystem::Instance().GetBodyID(ecs::entity(m_entity));
    if (bodyID.IsInvalid()) return;
    PhysicsSystem::Instance().GetBodyInterface().SetLinearVelocity(bodyID, PhysicsUtils::ToJolt(vel));
}

void RigidBodyComponent::SetLinearAndAngularVelocity(glm::vec3& inLinearVelocity, glm::vec3& inAngularVelocity) {
    JPH::BodyID bodyID = PhysicsSystem::Instance().GetBodyID(ecs::entity(m_entity));
    if (bodyID.IsInvalid()) return;
    PhysicsSystem::Instance().GetBodyInterface().SetLinearVelocity(bodyID, PhysicsUtils::ToJolt(inLinearVelocity));
    PhysicsSystem::Instance().GetBodyInterface().SetAngularVelocity(bodyID, PhysicsUtils::ToJolt(inAngularVelocity));
}

void RigidBodyComponent::AddLinearAndAngularVelocity(glm::vec3& inLinearVelocity, glm::vec3& inAngularVelocity) {
    JPH::BodyID bodyID = PhysicsSystem::Instance().GetBodyID(ecs::entity(m_entity));
    if (bodyID.IsInvalid()) return;
    PhysicsSystem::Instance().GetBodyInterface().AddLinearAndAngularVelocity(bodyID, PhysicsUtils::ToJolt(inLinearVelocity), PhysicsUtils::ToJolt(inAngularVelocity));
}

void RigidBodyComponent::SetPositionRotationAndVelocity(glm::vec3& inPosition, glm::quat& inRotation, glm::vec3& inLinearVelocity, glm::vec3& inAngularVelocity) {
    JPH::BodyID bodyID = PhysicsSystem::Instance().GetBodyID(ecs::entity(m_entity));
    if (bodyID.IsInvalid()) return;
    PhysicsSystem::Instance().GetBodyInterface().SetPositionRotationAndVelocity(bodyID, PhysicsUtils::ToJolt(inPosition), PhysicsUtils::ToQuatJolt(inRotation), PhysicsUtils::ToJolt(inLinearVelocity), PhysicsUtils::ToJolt(inAngularVelocity));
}

void RigidBodyComponent::MoveKinematic(const glm::vec3& targetPos, const glm::vec3& targetRot, const float duration) {
    JPH::BodyID bodyID = PhysicsSystem::Instance().GetBodyID(ecs::entity(m_entity));
    if (bodyID.IsInvalid())
    {
        spdlog::warn("Attempt to move an invalid body");
        return;
    }
    PhysicsSystem::Instance().GetBodyInterface().MoveKinematic(bodyID, PhysicsUtils::ToJolt(targetPos), PhysicsUtils::ToQuatJolt(targetRot), duration);
}

void RigidBodyComponent::SetAngularVelocity(const glm::vec3& angVel) {
    JPH::BodyID bodyID = PhysicsSystem::Instance().GetBodyID(ecs::entity(m_entity));
    if (bodyID.IsInvalid()) return;
    PhysicsSystem::Instance().GetBodyInterface().SetAngularVelocity(bodyID, PhysicsUtils::ToJolt(angVel));
}

void RigidBodyComponent::GetLinearAndAngularVelocity(glm::vec3& outLinearVelocity, glm::vec3& outAngularVelocity) {
    outAngularVelocity = angularVelocity;
    outLinearVelocity = linearVelocity;
}