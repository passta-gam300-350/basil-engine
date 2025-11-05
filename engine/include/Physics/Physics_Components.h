#pragma once
/*!************************************************************************
\file:      Physics_Components.h
\author:    Sam Tsang
\email:     sam.tsang@digipen.edu
\course:    CSD 3401 - Software Engineering Project 5
\brief:     This file has the definations of the components that will be used
by ecs for physics. They wrap a couple of jolt specific data types

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
***************************************************************************/

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Quat.h>
#include <vector>
#include <functional>

#include "ecs/internal/reflection.h"

struct RigidBodyComponent {
    JPH::BodyID bodyID;  // Jolt's body identifier
    JPH::EMotionType motionType;  // Static, Dynamic, or Kinematic

    // Cache commonly accessed data to avoid Jolt queries
    JPH::Vec3 velocity;
    JPH::Vec3 angularVelocity;
    float mass;

    bool isActive = true;
};


struct ColliderComponent {
    JPH::RefConst<JPH::Shape> shape;  // The collision shape
    JPH::Vec3 offset;  // Local offset from entity transform
    JPH::Quat rotation;  // Local rotation

    // Material properties
    float friction = 0.5f;
    float restitution = 0.0f;

    // Collision filtering
    uint32_t collisionMask;
};

struct CharacterControllerComponent {
    //JPH::Ref<JPH::Character> character;
    JPH::BodyID bodyID;

    // Character settings
    float maxSlopeAngle = 45.0f;  // degrees
    float mass = 70.0f;           // kg
    float friction = 0.5f;
    float gravityFactor = 1.0f;

    // Movement parameters
    float moveSpeed = 5.0f;       // m/s
    float jumpSpeed = 7.0f;       // m/s
    float capsuleHeight = 1.8f;   // meters
    float capsuleRadius = 0.3f;   // meters

    // State
    bool isOnGround = false;
    //glm::vec3 movementInput = glm::vec3(0.0f);
    bool wantsToJump = false;

    // Cached velocity
    JPH::Vec3 velocity = JPH::Vec3::sZero();
};

RegisterReflectionTypeBegin(RigidBodyComponent, "RigidBodyComponent")
    MemberRegistrationV<&RigidBodyComponent::bodyID, "bodyID">,
    MemberRegistrationV<&RigidBodyComponent::motionType, "motionType">,
    MemberRegistrationV<&RigidBodyComponent::velocity, "velocity">,
    MemberRegistrationV<&RigidBodyComponent::angularVelocity, "angularVelocity">,
    MemberRegistrationV<&RigidBodyComponent::mass, "mass">,
    MemberRegistrationV<&RigidBodyComponent::isActive, "isActive">
RegisterReflectionTypeEnd