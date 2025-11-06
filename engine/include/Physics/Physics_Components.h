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
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include "ecs/internal/reflection.h"
//#include <Jolt/Math/Vec3.h>
//#include <Jolt/Math/Quat.h>

struct RigidBodyComponent {
    uint32_t bodyID;  // Jolt's body identifier
    JPH::EMotionType motionType;  // Static, Dynamic, or Kinematic

    // Cache commonly accessed data to avoid Jolt queries
    JPH::Vec3 velocity;
    JPH::Vec3 angularVelocity;
    float mass;

    bool isActive = true;
};

struct Rigidbody {
    // Collision detection mode
    enum class CollisionDetectionMode {
        Discrete,              // Fast, can miss collisions
        Continuous,            // Slower, catches fast-moving objects
        ContinuousDynamic,     // Only continuous against static objects
        ContinuousSpeculative  // Predictive, best for high-speed
    };
    CollisionDetectionMode collisionDetection = CollisionDetectionMode::Discrete;

    // Interpolation for smooth rendering
    enum class InterpolationMode {
        None,         // No smoothing (can look jittery)
        Interpolate,  // Smooth between previous and current
        Extrapolate   // Predict next position
    };
    InterpolationMode interpolation = InterpolationMode::None;

    //----------------------------//
    // Variables
    //----------------------------//
    JPH::BodyID bodyID;            // Variables for keeping track of which Jolt Body Id is associated with this rigidbody

    // Motion properties
    float mass = 1.0f;              // Kilograms
    float drag = 0.0f;              // Linear drag (Air Resistance), 1.0000.0 = No air resistance (like in space) 0.5 = Moderate resistance(like moving through water) 5.0 = High resistance(like moving through honey)
    float angularDrag = 0.05f;      // Angular drag. Prevents objects from spinning forever.
    float friction = 0.0f;          
    float linearDamping = 0.0f;
    float gravityFactor = -9.81f;
    bool useGravity = true;         // Is gravity applied to the entity, true: Object falls down(default - 9.81 m / s²), false : Object floats(useful for flying enemies, UI elements in 3D space)
    bool isKinematic = false;       // Is this object controlled by animation/code or physics? false (Dynamic): Physics controls position (player can push it), true (Kinematic) : Your code controls position(moving platforms, doors)

    // Constraints for Position
    bool freezePositionX = false;
    bool freezePositionY = false;
    bool freezePositionZ = false;

    // Constrains for Rotation
    bool freezeRotationX = false;
    bool freezeRotationY = false;
    bool freezeRotationZ = false;

    // Cached physics state
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 angularVelocity = glm::vec3(0.0f);

    // Internal flags
    bool isActive = true;   // Controls whether this body participates in simulation
    bool isDirty = false;   // Needs sync to physics

    // Center of mass
    glm::vec3 centerOfMass = glm::vec3(0.0f);

    // Physics methods (to be called by scripts)
    void AddForce(const glm::vec3& force); // For Translation Force: Continuous effects (wind, magnets, thrust)
    void AddImpulse(const glm::vec3& force); // Impulse: Instant events (explosions, jumps, hits)
    void AddTorque(const glm::vec3& torque); // For Rotation, Force: Continuous effects (Gradually spin up a fan), 
    void AddAngularImpulse(const glm::vec3& Impulse); // Impulse: Instant events(Instantly spin a roulette wheel)
    void AddLinearVelocity(const glm::vec3& velocity); // Adds velocity to current velocity
    void SetLinearVelocity(const glm::vec3& newvel); // Changes velocity to new value
    void SetAngularVelocity(const glm::vec3& angVel); // Changes angular velocity to new value
    void MoveKinematic(const glm::vec3& targetPos, const glm::vec3& targetRotations, const float duration); // Moves the body to given position with a rotation given a duration

    // Acessor
    glm::vec3 GetVelocity() const noexcept { return velocity; }
    glm::vec3 GetAngularVelocity() const noexcept { return angularVelocity; }
};

// ============================================================================
// COLLISION DATA STRUCTURES
// ============================================================================

struct CollisionInfo {
    ecs::entity otherEntity;           // The entity we collided with
    glm::vec3 contactPoint;            // World space contact point
    glm::vec3 contactNormal;           // Normal pointing from other to this
    float penetrationDepth;            // How deep the collision is
    glm::vec3 relativeVelocity;        // Relative velocity at contact
    JPH::BodyID otherBodyID;           // Jolt body ID of other entity
};

struct TriggerInfo {
    ecs::entity otherEntity;           // The entity that entered/exited
    JPH::BodyID otherBodyID;           // Jolt body ID of other entity
};

// ============================================================================
// COLLISION DATA STRUCTURES
// ============================================================================
struct ContactPoint {
    glm::vec3 point;
    glm::vec3 normal;
    float separation;
    glm::vec3 impulse;
};

// ============================================================================
// COLLISION CALLBACK COMPONENT
// ============================================================================

struct CollisionCallbacks {
    // Collision callbacks (for solid colliders)
    std::function<void(const CollisionInfo&)> onCollisionEnter;
    std::function<void(const CollisionInfo&)> onCollisionStay;
    std::function<void(ecs::entity)> onCollisionExit;

    // Trigger callbacks (for trigger colliders)
    std::function<void(const TriggerInfo&)> onTriggerEnter;
    std::function<void(const TriggerInfo&)> onTriggerStay;
    std::function<void(ecs::entity)> onTriggerExit;
};

struct CapsuleCollider : public CollisionCallbacks
{
public:
    enum Direction
    {
        X_AXIS,
        Y_AXIS,
        Z_AXIS
    };

    /*!*************************************************************************
    Initializes the Collider component when created
    ****************************************************************************/
    CapsuleCollider();
    /*!*************************************************************************
    Initializes the Collider component when created, given another Collider
    component to move (for ECS)
    ****************************************************************************/
    CapsuleCollider(CapsuleCollider&& toMove) noexcept;
    /*!*************************************************************************
    Destructor for the Collider component class
    ****************************************************************************/
    ~CapsuleCollider() = default;
    /*!*************************************************************************
    Getter and setter functions for the variables in the Collider component class
    ****************************************************************************/
    bool& GetIsTrigger() { return mIsTrigger; }
    void SetIsTrigger(bool isTrigger) { mIsTrigger = isTrigger; }

    JPH::Vec3& GetCenter() { return mCenter; }
    void SetCenter(JPH::Vec3 center) { mCenter = center; }
    void SetCenter(float x, float y, float z) { mCenter = JPH::Vec3(x, y, z); }

    float& GetRadius() { return mRadius; }
    void SetRadius(float radius) { mRadius = radius; }

    float& GetHeight() { return mHeight; }
    void SetHeight(float height) { mHeight = height; }

    Direction& GetDirection() { return mDirection; }
    void SetDirection(Direction direction) { mDirection = direction; }

private:
    bool mIsTrigger;
    JPH::Vec3 mCenter;
    float mRadius;
    float mHeight;
    Direction mDirection;
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


RegisterReflectionTypeBegin(RigidBodyComponent, "RigidBodyComponent")
    MemberRegistrationV<&RigidBodyComponent::bodyID, "bodyID">,
    MemberRegistrationV<&RigidBodyComponent::motionType, "motionType">,
    MemberRegistrationV<&RigidBodyComponent::velocity, "velocity">,
    MemberRegistrationV<&RigidBodyComponent::angularVelocity, "angularVelocity">,
    MemberRegistrationV<&RigidBodyComponent::mass, "mass">,
    MemberRegistrationV<&RigidBodyComponent::isActive, "isActive">
RegisterReflectionTypeEnd