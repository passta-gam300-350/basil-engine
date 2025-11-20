#pragma once
/*!************************************************************************
\file:      Physics_System.h
\author:    Sam Tsang
\email:     sam.tsang@digipen.edu
\course:    CSD 3401 - Software Engineering Project 5
\brief:     This file has the declaration of the physics engine that wraps around the jolt library
as well as a couple of helper functions for converting vec3's from glm to jolt

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
***************************************************************************/

// Jolt includes
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

// STL includes
#include <iostream>
#include <cstdarg>
#include <thread>

#include <memory>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

#include "System/System.hpp"
#include "Ecs/ecs.h"
#include "Physics/Physics_Components.h"
#include "components/transform.h"
#include <glm/gtc/quaternion.hpp>  

JPH_SUPPRESS_WARNINGS

// Notes for jolt, scaling is relative to the middle, so a scale of 1.0 results and a cube of side 2.0.


// Collision layers (customize for your game)
namespace Layers {
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}

namespace BroadPhaseLayers {
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::uint NUM_LAYERS(2);
}

// Forward declarations
namespace JPH {
    class TempAllocatorImpl;
    class JobSystemThreadPool;
}


struct PhysicsSystem : public ecs::SystemBase {
public:
    bool isActive = true;

public:
    // SystemBase interface implementation
    void Init();
    void FixedUpdate(ecs::world& world);
    void Exit();
    
    static std::unique_ptr<PhysicsSystem>& InstancePtr(){
        static std::unique_ptr<PhysicsSystem> sys{ std::make_unique<PhysicsSystem>() };
        return sys;
    }

    static PhysicsSystem& Instance() {
        return *InstancePtr();
    }

    // Helper: Create body for entity if it has both Rigidbody + Collider
    void TryCreateBodyForEntity(ecs::entity entity);
    //JPH::BodyID CreateRigidBody(ecs::world& world, const RigidBodyComponent& rb, const TransformComponent& trans, const ColliderComponent& collider);
    void DestroyRigidBody(JPH::BodyID bodyID);

    // Accessors
    JPH::PhysicsSystem* GetJoltPhysicsSystem() noexcept { return m_physicsSystem.get(); } // Returns a pointer to the Jolt Engine
    JPH::BodyInterface& GetBodyInterface() noexcept { return *m_bodyInterface; } // Returns a reference to the Body Interface

    
    void SyncTransformsToPhysics(ecs::world& world);        // 1. Syncs transform data from ecs to internal jolt data
                                                            // 2. Calls the update for jolt's internal engine, collision call back are also called by jolt when it detect collision
    void SyncTransformsFromPhysics(ecs::world& world);      // 3. Syncs the new internal jolt data to transform
    void ProcessCollisionEvents(ecs::world& world);         // 4. Process the queue of collision events to prevent conflict like deleting an entity if it is the result of a collision callback

    // New: Get BodyID for an entity (returns invalid if not found)
    JPH::BodyID GetBodyID(ecs::entity entity) const;

    // New: Setup observers for automatic body creation/destruction
    void SetupObservers();
    void DisableObservers();  // Disconnect all observers
    void EnableObservers();   // Reconnect all observers

    // Batch creation for scene loading
    void CreateAllBodiesForLoadedScene();
    void DestroyAllBodiesForUnload();

    // Reset physics system (cleanup all bodies and mappings)
    void Reset();

    // Collision/Trigger handlers (called by ContactListener)
    void HandleCollisionEnter(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold);
    void HandleCollisionStay(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold);
    void HandleCollisionExit(const JPH::SubShapeIDPair& subShapePair);

    // Debug Function for checking jolt's position
    void PrintDebugInfo(ecs::entity entity);

    // ===== RAYCASTING API =====

    // Raycast result structure
    struct RaycastHit {
        bool hasHit = false;              // Did the ray hit anything?
        ecs::entity entity;               // Entity that was hit
        glm::vec3 hitPoint;               // World space hit point
        glm::vec3 hitNormal;              // Surface normal at hit point
        float distance = 0.0f;            // Distance from ray origin to hit point
        JPH::BodyID bodyID;               // Jolt body ID (for advanced use)
        bool isTrigger = false;           // Was the hit object a trigger?
    };

    // Simple raycast - returns first hit
    RaycastHit Raycast(
        const glm::vec3& origin,
        const glm::vec3& direction,
        float maxDistance = 1000.0f,
        bool ignoreTriggers = true
    );

    // Raycast with entity ignore list
    RaycastHit RaycastIgnoring(
        const glm::vec3& origin,
        const glm::vec3& direction,
        const std::vector<ecs::entity>& ignoreEntities,
        float maxDistance = 1000.0f,
        bool ignoreTriggers = true
    );

    // Raycast returning all hits along the ray
    std::vector<RaycastHit> RaycastAll(
        const glm::vec3& origin,
        const glm::vec3& direction,
        float maxDistance = 1000.0f,
        bool ignoreTriggers = true
    );

private:

    // Funcitons for updating the jolt body's through the editor
    void SyncDirtyRigidBodies(ecs::world& world);
    void SyncDirtyColliders(ecs::world& world);
    void UpdateBodyProperties(JPH::BodyID bodyID, const RigidBodyComponent& rb);    // Update individual body properties
    void RecreateBodyWithNewShape(ecs::entity entity, ecs::world& world);           // Recreate body with new shape

    // Observer callbacks
    void OnRigidbodyAdded(entt::registry& registry, entt::entity entity);
    void OnRigidbodyDestroyed(entt::registry& registry, entt::entity entity);
    void OnColliderAdded(entt::registry& registry, entt::entity entity);
    void OnColliderDestroyed(entt::registry& registry, entt::entity entity);



    // New: Get entity from BodyID (reverse lookup)
    ecs::entity GetEntityFromBodyID(JPH::BodyID bodyID) const;

    // Helper: Check if entity has trigger collider
    bool IsEntityTrigger(ecs::entity entity);

    // Helper: Build collision info from manifold
    CollisionInfo BuildCollisionInfo(
        ecs::entity otherEntity,
        const JPH::Body& otherBody,
        const JPH::ContactManifold& manifold,
        bool flipNormal
    );

    // Helper: Build trigger info
    TriggerInfo BuildTriggerInfo(ecs::entity otherEntity, JPH::BodyID otherBodyID);

    // Template helper: Invoke callbacks on any collider type
    template<typename EventType>
    void InvokeColliderCallback(ecs::entity entity, const EventType& eventData, const char* callbackType);

    JPH::RefConst<JPH::Shape> CreateShapeFromCollider(ecs::entity entity);


    bool m_observersEnabled = false; // Track Observer state

    // Jolt core objects
    std::unique_ptr<JPH::TempAllocatorImpl> m_tempAllocator; // Used for Jolt Update
    std::unique_ptr<JPH::JobSystemThreadPool> m_jobSystem; // Used for Jolt Update
    JPH::BodyInterface* m_bodyInterface = nullptr; // Non-owning pointer, Used for accessing Jolt's body's paramemters
    std::unique_ptr<JPH::PhysicsSystem> m_physicsSystem; // Used for keeping Jolt's engine

    // Contact listener
    std::unique_ptr<JPH::ContactListener> m_contactListener;

    // Mapping between entities and bodies
    std::vector<JPH::BodyID> m_JoltBodyIDs;
    std::unordered_map<ecs::entity, JPH::BodyID> m_entityToBodyID;    // EntityId to BodyID map
    std::unordered_map<uint32_t, ecs::entity> m_bodyIDToEntity;    // BodyID index to Entity UID
};

// Utils for conveerting vectors, mat4 and angles from glm to jolt and vice versa
namespace PhysicsUtils {
    // GLM to Jolt
    inline JPH::Vec3 ToJolt(const glm::vec3& v) {
        return JPH::Vec3(v.x, v.y, v.z);
    }

    // Jolt to GLM
    inline glm::vec3 ToGLM(const JPH::Vec3& v) {
        return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
    }

    // GLM quat to Jolt quat
    inline JPH::Quat ToQuatJolt(const glm::quat& q) {
        return JPH::Quat(q.x, q.y, q.z, q.w);
    }

    // Jolt quat to GLM quat
    inline glm::quat ToQuatGLM(const JPH::Quat& q) {
        return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
    }

    // GLM mat4 to Jolt Mat44
    inline JPH::Mat44 ToMatJolt(const glm::mat4& m) {
        return JPH::Mat44(
            JPH::Vec4(m[0][0], m[0][1], m[0][2], m[0][3]),
            JPH::Vec4(m[1][0], m[1][1], m[1][2], m[1][3]),
            JPH::Vec4(m[2][0], m[2][1], m[2][2], m[2][3]),
            JPH::Vec4(m[3][0], m[3][1], m[3][2], m[3][3])
        );
    }

    // Jolt Mat44 to GLM mat4
    inline glm::mat4 ToMatGLM(const JPH::Mat44& m) {
        return glm::mat4(
            m(0, 0), m(0, 1), m(0, 2), m(0, 3),
            m(1, 0), m(1, 1), m(1, 2), m(1, 3),
            m(2, 0), m(2, 1), m(2, 2), m(2, 3),
            m(3, 0), m(3, 1), m(3, 2), m(3, 3)
        );
    }

    // Convert Euler angles (in radians) to Jolt quaternion
    inline JPH::Quat EulerToJoltQuat(const glm::vec3& eulerRadians) {
        // Create GLM quaternion from Euler angles
        glm::quat const glmQuat = glm::quat(eulerRadians);

        // Convert to Jolt
        return JPH::Quat(glmQuat.x, glmQuat.y, glmQuat.z, glmQuat.w);
    }

    // If your angles are in degrees:
    inline JPH::Quat EulerDegreesToJoltQuat(const glm::vec3& eulerDegrees) {
        glm::vec3 const eulerRadians = glm::radians(eulerDegrees);
        glm::quat const glmQuat = glm::quat(eulerRadians);
        return JPH::Quat(glmQuat.x, glmQuat.y, glmQuat.z, glmQuat.w);
    }

    // Convert Jolt quaternion to Euler angles (radians)
    inline glm::vec3 JoltQuatToEuler(const JPH::Quat& q) {
        glm::quat const glmQuat = ToQuatGLM(q);
        return glm::eulerAngles(glmQuat);  // Returns radians
    }

    // Convert Jolt quaternion to Euler angles (degrees)
    inline glm::vec3 JoltQuatToEulerDegrees(const JPH::Quat& q) {
        glm::vec3 const eulerRadians = JoltQuatToEuler(q);
        return glm::degrees(eulerRadians);
    }
}