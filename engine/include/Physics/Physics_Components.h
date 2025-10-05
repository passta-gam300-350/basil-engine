#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Quat.h>
#include <vector>
#include <functional>


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