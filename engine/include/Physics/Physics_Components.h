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
    // Motion type (serializable enum instead of JPH::EMotionType)
    enum class MotionType : uint8_t {
        Static,      // Doesn't move (walls, floors)
        Dynamic,     // Fully simulated by physics
        Kinematic    // Controlled by animation/code
    };
    
    // Convert to Jolt motion type
    JPH::EMotionType ToJoltMotionType() const {
        switch (motionType) {
        case MotionType::Static: return JPH::EMotionType::Static;
        case MotionType::Kinematic: return JPH::EMotionType::Kinematic;
        case MotionType::Dynamic: return JPH::EMotionType::Dynamic;
        default: return JPH::EMotionType::Dynamic;
        }
    }

    MotionType ToPhysicsMotionType(JPH::EMotionType mType) {
        switch (mType) {
        case JPH::EMotionType::Static: return MotionType::Static;
        case JPH::EMotionType::Kinematic: return MotionType::Kinematic;
        case JPH::EMotionType::Dynamic: return MotionType::Dynamic;
        default: return MotionType::Dynamic;
        }
    }

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
    ecs::entity m_entity;            // Set when body is created, used for checking which BodyId is associated with this entity in the physics system internal map
    MotionType motionType = MotionType::Dynamic;
     
    // Motion properties
    float mass = 1.0f;              // Kilograms
    float drag = 0.0f;              // Linear drag (Air Resistance), 1.0000.0 = No air resistance (like in space) 0.5 = Moderate resistance(like moving through water) 5.0 = High resistance(like moving through honey)
    float angularDrag = 0.05f;      // Angular drag. Prevents objects from spinning forever.
    float friction = 0.0f;          
    float linearDamping = 0.0f;
    float gravityFactor = -9.81f; 
    bool useGravity = true;         // Is gravity applied to the entity, true: Object falls down(default - 9.81 m/s^2), false : Object floats(useful for flying enemies, UI elements in 3D space)
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
    glm::vec3 linearVelocity = glm::vec3(0.0f);
    glm::vec3 angularVelocity = glm::vec3(0.0f);

    // Internal flags
    bool isActive = true;   // Controls whether this body participates in simulation
    bool isDirty = false;   // Needs sync to physics
     
    // Center of mass
    glm::vec3 centerOfMass = glm::vec3(0.0f);

    // Helper to determine if this should be static
    bool IsStatic() const {
        return motionType == MotionType::Static;
    }

    // Physics methods (to be called by scripts)
    void AddForce(const glm::vec3& force); // For Translation Force: Continuous effects (wind, magnets, thrust)
    void AddImpulse(const glm::vec3& force); // Impulse: Instant events (explosions, jumps, hits)
    void AddTorque(const glm::vec3& torque); // For Rotation, Force: Continuous effects (Gradually spin up a fan), 
    void AddAngularImpulse(const glm::vec3& Impulse); // Impulse: Instant events(Instantly spin a roulette wheel)
    void SetAngularVelocity(const glm::vec3& angVel); // Changes angular velocity to new value
    void AddLinearVelocity(const glm::vec3& velocity); // Adds velocity to current velocity
    void SetLinearVelocity(const glm::vec3& newvel); // Changes velocity to new value
    
    void SetLinearAndAngularVelocity(glm::vec3& inLinearVelocity, glm::vec3& inAngularVelocity);
    void GetLinearAndAngularVelocity(glm::vec3& outLinearVelocity, glm::vec3& outAngularVelocity);
    void AddLinearAndAngularVelocity(glm::vec3& inLinearVelocity, glm::vec3& inAngularVelocity);                                                  // 
    void SetPositionRotationAndVelocity(glm::vec3& inPosition, glm::quat& inRotation, glm::vec3& inLinearVelocity, glm::vec3& inAngularVelocity); // Sets Position, Rotation and Velocity
    
    void MoveKinematic(const glm::vec3& targetPos, const glm::vec3& targetRotations, const float duration); // Moves the body to given position with a rotation given a duration

    // Acessor
    const glm::vec3& GetLinearVelocity() const noexcept { return linearVelocity; }
    glm::vec3& GetLinearVelocity() noexcept { return linearVelocity; };

    const glm::vec3& GetAngularVelocity() const noexcept { return angularVelocity; }
    glm::vec3& GetAngularVelocity() noexcept { return angularVelocity; };
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

struct CollisionBase {
    // Collision callbacks (for solid colliders)
    std::function<void(const CollisionInfo&)> onCollisionEnter;
    std::function<void(const CollisionInfo&)> onCollisionStay;
    std::function<void(ecs::entity)> onCollisionExit;

    // Trigger callbacks (for trigger colliders)
    std::function<void(const TriggerInfo&)> onTriggerEnter;
    std::function<void(const TriggerInfo&)> onTriggerStay;
    std::function<void(ecs::entity)> onTriggerExit;

    // Common properties
    glm::vec3 center = glm::vec3(0.0f);      // Local offset
    bool isTrigger = false;

    // Physics material
    float friction = 0.5f;
    float restitution = 0.0f;                // Bounciness
    float density = 1.0f;                    // For auto mass calculation

    // Runtime state
    bool isDirty = true;                     // Needs recreation

    bool& GetIsTrigger() { return isTrigger; }
    void SetIsTrigger(bool newTrigger) { isTrigger = newTrigger; }

    glm::vec3& GetCenter() { return center; }
    void SetCenter(glm::vec3 newCenter) { center = newCenter; }
    void SetCenter(float x, float y, float z) { center = glm::vec3(x, y, z); }
};

// Specific collider types
struct BoxCollider : public CollisionBase {
    glm::vec3 size = glm::vec3(1.0f);       // Full size (not half-extents)
};

struct SphereCollider : public CollisionBase {
    float radius = 0.5f;
};

struct CapsuleCollider : public CollisionBase
{
public:
    enum Direction
    {
        X_AXIS,
        Y_AXIS,
        Z_AXIS
    };


    /*!*************************************************************************
    Getter and setter functions for the variables in the Collider component class
    ****************************************************************************/

    float& GetRadius() noexcept { return mRadius; }
    void SetRadius(float radius) noexcept { mRadius = radius; }

    float& GetHeight() noexcept { return mHeight; }
    void SetHeight(float height) noexcept { mHeight = height; }

    Direction& GetDirection() noexcept { return mDirection; }
    void SetDirection(Direction direction) noexcept { mDirection = direction; }

    float mRadius;
    float mHeight;
    Direction mDirection;
private:

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
    MemberRegistrationV<&RigidBodyComponent::motionType, "motionType">,
    MemberRegistrationV<&RigidBodyComponent::mass, "mass">,
    MemberRegistrationV<&RigidBodyComponent::drag, "drag">,
    MemberRegistrationV<&RigidBodyComponent::angularDrag, "angularDrag">,
    MemberRegistrationV<&RigidBodyComponent::friction, "friction">,
    MemberRegistrationV<&RigidBodyComponent::linearDamping, "linearDamping">,
    MemberRegistrationV<&RigidBodyComponent::gravityFactor, "gravityFactor">,
    MemberRegistrationV<&RigidBodyComponent::useGravity, "useGravity">,
    MemberRegistrationV<&RigidBodyComponent::isKinematic, "isKinematic">,
    MemberRegistrationV<&RigidBodyComponent::freezePositionX, "freezePositionX">,
    MemberRegistrationV<&RigidBodyComponent::freezePositionY, "freezePositionY">,
    MemberRegistrationV<&RigidBodyComponent::freezePositionZ, "freezePositionZ">,
    MemberRegistrationV<&RigidBodyComponent::freezeRotationX, "freezeRotationX">,
    MemberRegistrationV<&RigidBodyComponent::freezeRotationY, "freezeRotationY">,
    MemberRegistrationV<&RigidBodyComponent::freezeRotationZ, "freezeRotationZ">,
    MemberRegistrationV<&RigidBodyComponent::centerOfMass, "centerOfMass">,
    MemberRegistrationV<&RigidBodyComponent::linearVelocity, "linearVelocity">,
    MemberRegistrationV<&RigidBodyComponent::angularVelocity, "angularVelocity">,
    MemberRegistrationV<&RigidBodyComponent::mass, "mass">,
    MemberRegistrationV<&RigidBodyComponent::isActive, "isActive">
RegisterReflectionTypeEnd

RegisterReflectionTypeBegin(CapsuleCollider, "CapsuleCollider")
    MemberRegistrationV<&CapsuleCollider::center, "CenterOffset">,
    MemberRegistrationV<&CapsuleCollider::mRadius, "Radius">,
    MemberRegistrationV<&CapsuleCollider::mHeight, "Height">,
    MemberRegistrationV<&CapsuleCollider::isTrigger, "isTrigger">,
    MemberRegistrationV<&CapsuleCollider::mDirection, "Direction">,
    MemberRegistrationV<&CapsuleCollider::friction, "friction">,
    MemberRegistrationV<&CapsuleCollider::restitution, "restitution">,
    MemberRegistrationV<&CapsuleCollider::density, "density">
RegisterReflectionTypeEnd

RegisterReflectionTypeBegin(BoxCollider, "BoxCollider")
    MemberRegistrationV<&BoxCollider::size, "size">,
    MemberRegistrationV<&BoxCollider::center, "CenterOffset">,
    MemberRegistrationV<&BoxCollider::isTrigger, "isTrigger">,
    MemberRegistrationV<&BoxCollider::friction, "friction">,
    MemberRegistrationV<&BoxCollider::restitution, "restitution">,
    MemberRegistrationV<&BoxCollider::density, "density">
RegisterReflectionTypeEnd