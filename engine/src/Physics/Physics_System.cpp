/*!************************************************************************
\file:      Physics_System.cpp
\author:    Sam Tsang
\email:     sam.tsang@digipen.edu
\course:    CSD 3401 - Software Engineering Project 5
\brief:     This file has the implementation of the physics engine that wraps around the jolt library

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
***************************************************************************/

#include "Physics/Physics_System.h"
#include "Profiler/profiler.hpp"


// Callback for traces, connect this to your own trace function if you have one
static void TraceImpl(const char* inFMT, ...)
{
    // Format the message
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);

    // Print to the TTY
    std::cout << buffer << std::endl;
}

//#ifdef JPH_ENABLE_ASSERTS
//
//// Callback for asserts, connect this to your own assert handler if you have one
//static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine)
//{
//    // Print to the TTY
//    std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << std::endl;
//
//    // Breakpoint
//    return true;
//};
//
//#endif // JPH_ENABLE_ASSERTS


// BroadPhaseLayerInterface implementation
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
public:
    BPLayerInterfaceImpl() {
        m_objectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        m_objectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }

    virtual JPH::uint GetNumBroadPhaseLayers() const override {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
        return m_objectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        switch ((JPH::BroadPhaseLayer::Type)inLayer)
        {
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	
                return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		
                return "MOVING";
            default:									   
                JPH_ASSERT(false); return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
    JPH::BroadPhaseLayer m_objectToBroadPhase[Layers::NUM_LAYERS];
};

// ObjectVsBroadPhaseLayerFilter implementation
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
        switch (inLayer1) {
        case Layers::NON_MOVING:
            return inLayer2 == BroadPhaseLayers::MOVING;
        case Layers::MOVING:
            return true;
        default:
            return false;
        }
    }
};

// ObjectLayerPairFilter implementation
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
        switch (inObject1) {
        case Layers::NON_MOVING:
            return inObject2 == Layers::MOVING;
        case Layers::MOVING:
            return true;
        default:
            return false;
        }
    }
};

// Contact listener for collision events
class MyContactListener : public JPH::ContactListener {
public:
    MyContactListener(PhysicsSystem* physicsSystem) : m_physicsSystem(physicsSystem) {}

    // Pre contact, no unity equivillent 
    virtual JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override {
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    // Unity OnContact
    virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override {
        m_physicsSystem->HandleCollisionEnter(inBody1, inBody2, inManifold);
    }

    // Unity OnStay
    virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override {
        m_physicsSystem->HandleCollisionStay(inBody1, inBody2, inManifold);
    }

    // Unity OnLeave
    virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override {
        m_physicsSystem->HandleCollisionExit(inSubShapePair);
    }

private:
    PhysicsSystem* m_physicsSystem;
};

// Parameters for Physics System
// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
const JPH::uint cMaxBodies = 65536;

// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
const JPH::uint cNumBodyMutexes = 0;

// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
const JPH::uint cMaxBodyPairs = 65536;

// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
const JPH::uint cMaxContactConstraints = 10240;

BPLayerInterfaceImpl						broad_phase_layer_interface;
ObjectVsBroadPhaseLayerFilterImpl			object_vs_broadphase_layer_filter;
ObjectLayerPairFilterImpl					object_vs_object_layer_filter;


void PhysicsSystem::Init() {
    // Register allocation hook
    JPH::RegisterDefaultAllocator();

    // Install callbacks
    JPH::Trace = TraceImpl;
    //JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)


    // Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
    JPH::Factory::sInstance = new JPH::Factory();

    // Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
    // If you have your own custom shape types you probably need to register their handlers with the CollisionDispatch before calling this function.
    // If you implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this function will create one for you.
    JPH::RegisterTypes();

    // Create temp allocator (10 MB)
    m_tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

    // Create job system
    m_jobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);


    // Now we can create the actual physics system.
    m_physicsSystem = std::make_unique<JPH::PhysicsSystem>();
    m_physicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);

    // Get body interface (non-owning pointer)
    m_bodyInterface = &m_physicsSystem->GetBodyInterface();

    // Create contact listener
    m_contactListener = std::make_unique<MyContactListener>(this);
    m_physicsSystem->SetContactListener(m_contactListener.get());

    // Set gravity
    m_physicsSystem->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

    auto entity = Engine::GetWorld().add_entity();
    Engine::GetWorld().add_component_to_entity<BoxCollider>(entity);
    Engine::GetWorld().remove_entity(entity);
}


void PhysicsSystem::FixedUpdate(ecs::world& world) {
    PF_SYSTEM("PhysicsSystem");
    if (!m_physicsSystem || !m_bodyInterface || !isActive) return;

    const float deltaTime = 1.0f / 60.0f;

    // 2. Sync transforms from ECS to physics (for kinematic bodies)
    SyncTransformsToPhysics(world);

    // 3. Step the physics simulation
    const int collisionSteps = 1;
    m_physicsSystem->Update(deltaTime, collisionSteps, m_tempAllocator.get(), m_jobSystem.get());

    // 4. Sync transforms from physics back to ECS (for dynamic bodies)
    SyncTransformsFromPhysics(world);

    // 5. Process collision events
    ProcessCollisionEvents(world);
}

void PhysicsSystem::SyncTransformsToPhysics(ecs::world& world) {
    if (!m_bodyInterface) return;

    // Update kinematic bodies from ECS transforms
    auto list_of_entities = world.filter_entities<RigidBodyComponent, TransformComponent>();

    for (auto const& entity : list_of_entities) 
    {
        if (!world.has_any_components_in_entity<BoxCollider, SphereCollider, CapsuleCollider>(entity)) { continue; }
        auto [RigidBody, Transform] {entity.get<RigidBodyComponent, TransformComponent>()};
        auto Position = m_bodyInterface->GetPosition(m_entityToBodyID[entity]);

        spdlog::warn("PhysicsSystem: current Updating bodyId {} for entity {}", m_entityToBodyID[entity].GetIndexAndSequenceNumber(), entity.get_uuid());

        m_bodyInterface->SetPositionAndRotation(m_entityToBodyID[entity], PhysicsUtils::ToJolt(Transform.m_Translation), PhysicsUtils::EulerDegreesToJoltQuat(Transform.m_Rotation), JPH::EActivation::Activate);

        auto Position2 = m_bodyInterface->GetPosition(m_entityToBodyID[entity]);
        spdlog::warn("Prevous Pos: {}, New Pos: {}, Trans Pos: {}", Position.GetX(), Position2.GetX(), Transform.m_Translation.x);
    }
}

void PhysicsSystem::SyncTransformsFromPhysics(ecs::world& world) {
    if (!m_bodyInterface) return;
   
    // Update ECS transforms from dynamic bodies
    auto list_of_entities = world.filter_entities<RigidBodyComponent, TransformComponent>();

    for (auto const& entity : list_of_entities) 
    {
        if (!world.has_any_components_in_entity<BoxCollider, SphereCollider, CapsuleCollider>(entity)) { continue; }
        auto [RigidBody, Transform] {entity.get<RigidBodyComponent, TransformComponent>()};

        if ((RigidBody.motionType == RigidBodyComponent::MotionType::Dynamic) && RigidBody.isActive) {
            JPH::BodyID BodyId = m_entityToBodyID[entity];
            JPH::Vec3 position;
            JPH::Quat rotation;
            m_bodyInterface->GetPositionAndRotation(BodyId, position, rotation);

            Transform.m_Translation = PhysicsUtils::ToGLM(position);
            Transform.m_Rotation = PhysicsUtils::JoltQuatToEulerDegrees(rotation);

            // Update cached velocity
            RigidBody.linearVelocity = PhysicsUtils::ToGLM(m_bodyInterface->GetLinearVelocity(BodyId));
            RigidBody.angularVelocity = PhysicsUtils::ToGLM(m_bodyInterface->GetAngularVelocity(BodyId));
        }
    }
}

void PhysicsSystem::ProcessCollisionEvents(ecs::world&) {
    // Process any queued collision events
    // This depends on how you want to handle events in your ECS
    // You might dispatch events, set flags on components, etc.
}

// Creates the body of a specific shape and returns the new bodyID
JPH::BodyID PhysicsSystem::CreateRigidBody(ecs::world& world, const RigidBodyComponent& rb, const TransformComponent&
    trans, const ColliderComponent& collider) {
    if (!m_bodyInterface) return JPH::BodyID();

    JPH::RefConst<JPH::Shape> shape = collider.shape;

    // Create body settings using Rigidbody's motion type
    JPH::BodyCreationSettings bodySettings(
        shape,
        PhysicsUtils::ToJolt(trans.m_Translation),
        PhysicsUtils::EulerDegreesToJoltQuat(trans.m_Rotation),
        rb.ToJoltMotionType(),
        rb.IsStatic() ? Layers::NON_MOVING : Layers::MOVING
    );

    // Set mass properties
    if (rb.motionType == RigidBodyComponent::MotionType::Dynamic) {
        bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateMassAndInertia;
        bodySettings.mMassPropertiesOverride.mMass = rb.mass;
    }

    bodySettings.mFriction = rb.friction;  // Use Rigidbody's friction
    bodySettings.mRestitution = collider.restitution;
    bodySettings.mLinearDamping = rb.linearDamping;
    bodySettings.mAngularDamping = rb.angularDrag;
    bodySettings.mGravityFactor = rb.useGravity ? rb.gravityFactor : 0.0f;

    // Create the body
    JPH::Body* body = m_bodyInterface->CreateBody(bodySettings);
    if (!body) {
        spdlog::error("PhysicsSystem: Failed to create body");
        return JPH::BodyID();
    }

    JPH::BodyID bodyID = body->GetID();

    // Add to physics world
    m_bodyInterface->AddBody(bodyID, rb.isActive ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);

    // Note: Don't add to m_JoltBodyIDs vector anymore, it's stored in m_entityToBodyID map

    return bodyID;
}

void PhysicsSystem::DestroyRigidBody(JPH::BodyID bodyID) {
    if (bodyID.IsInvalid() || !m_bodyInterface) return;

    // Remove and destroy body
    m_bodyInterface->RemoveBody(bodyID);
    m_bodyInterface->DestroyBody(bodyID);
}

void PhysicsSystem::Exit() {

    // Destroy all bodies
    if (m_bodyInterface) {
        for (auto& bodyID : m_JoltBodyIDs) {
            m_bodyInterface->RemoveBody(bodyID);
            m_bodyInterface->DestroyBody(bodyID);
        }
    }
    m_JoltBodyIDs.clear();

    // Clear body interface pointer before destroying physics system
    m_bodyInterface = nullptr;

    // Clean up Jolt (unique_ptrs handle deletion automatically in reverse order)
    m_contactListener.reset();
    m_physicsSystem.reset();
    m_jobSystem.reset();
    m_tempAllocator.reset();

    // Unregister types and clean up factory
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

void PhysicsSystem::SetupObservers() {
    if (m_observersEnabled) {
        spdlog::warn("PhysicsSystem: Observers already enabled");
        return;
    }
    EnableObservers();
   
}

void PhysicsSystem::EnableObservers() {
    if (m_observersEnabled) {
        return;  // Already enabled
    }
    auto world = Engine::GetWorld();
    entt::registry& registry = world.impl.get_registry();

    // Disconnect first to ensure clean state
    registry.on_construct<RigidBodyComponent>().disconnect();
    registry.on_destroy<RigidBodyComponent>().disconnect();
    registry.on_construct<BoxCollider>().disconnect();
    registry.on_destroy<BoxCollider>().disconnect();
    registry.on_construct<SphereCollider>().disconnect();
    registry.on_destroy<SphereCollider>().disconnect();
    registry.on_construct<CapsuleCollider>().disconnect();
    registry.on_destroy<CapsuleCollider>().disconnect();

    // Connect observers
    registry.on_construct<RigidBodyComponent>().connect<&PhysicsSystem::OnRigidbodyAdded>(this);
    registry.on_destroy<RigidBodyComponent>().connect<&PhysicsSystem::OnRigidbodyDestroyed>(this);
    registry.on_construct<BoxCollider>().connect<&PhysicsSystem::OnColliderAdded>(this);
    registry.on_destroy<BoxCollider>().connect<&PhysicsSystem::OnColliderDestroyed>(this);
    registry.on_construct<SphereCollider>().connect<&PhysicsSystem::OnColliderAdded>(this);
    registry.on_destroy<SphereCollider>().connect<&PhysicsSystem::OnColliderDestroyed>(this);
    registry.on_construct<CapsuleCollider>().connect<&PhysicsSystem::OnColliderAdded>(this);
    registry.on_destroy<CapsuleCollider>().connect<&PhysicsSystem::OnColliderDestroyed>(this);

    m_observersEnabled = true;
    spdlog::info("PhysicsSystem: Observers enabled");
}

void PhysicsSystem::DisableObservers() {
    if (!m_observersEnabled) {
        return;  // Already disabled
    }
    auto world = Engine::GetWorld();
    entt::registry& registry = world.impl.get_registry();

    // Disconnect all observers
    registry.on_construct<RigidBodyComponent>().disconnect();
    registry.on_destroy<RigidBodyComponent>().disconnect();
    registry.on_construct<BoxCollider>().disconnect();
    registry.on_destroy<BoxCollider>().disconnect();
    registry.on_construct<SphereCollider>().disconnect();
    registry.on_destroy<SphereCollider>().disconnect();
    registry.on_construct<CapsuleCollider>().disconnect();
    registry.on_destroy<CapsuleCollider>().disconnect();

    m_observersEnabled = false;
    spdlog::debug("PhysicsSystem: Observers disabled");
}

void PhysicsSystem::OnRigidbodyAdded(entt::registry& registry, entt::entity entity) {
    // Convert to ecs::entity
    ecs::entity const ecsEntity = Engine::GetWorld().impl.entity_cast(entity);
    auto world = Engine::GetWorld();
    // Only create body if entity also has ColliderComponent
    if (world.has_any_components_in_entity<ColliderComponent, BoxCollider, CapsuleCollider, SphereCollider>(ecsEntity))
    {
        TryCreateBodyForEntity(ecsEntity);
    }
}

void PhysicsSystem::OnColliderAdded(entt::registry& registry, entt::entity entity) {
    
    ecs::entity ecsEntity = Engine::GetWorld().impl.entity_cast(entity);
    auto world = Engine::GetWorld();
    // Only create body if entity also has RigidBodyComponent
    if (world.has_any_components_in_entity<RigidBodyComponent>(ecsEntity))
    {
        TryCreateBodyForEntity(ecsEntity);
    }
}

void PhysicsSystem::OnRigidbodyDestroyed(entt::registry& registry, entt::entity entity) {
    uint64_t uid = static_cast<uint64_t>(ecs::world::detail::entity_id_cast(entity));

    // Find and destroy associated Jolt body
    auto it = m_entityToBodyID.find(uid);
    if (it != m_entityToBodyID.end()) {
        DestroyRigidBody(it->second);
        m_entityToBodyID.erase(it);
    }
}

void PhysicsSystem::OnColliderDestroyed(entt::registry& registry, entt::entity entity) {
    // Collider removed = body needs recreation (or destruction if no Rigidbody)
    uint64_t uid = static_cast<uint64_t>(ecs::world::detail::entity_id_cast(entity));

    auto it = m_entityToBodyID.find(uid);
    if (it != m_entityToBodyID.end()) {
        DestroyRigidBody(it->second);
        m_entityToBodyID.erase(it);
    }
}

void PhysicsSystem::TryCreateBodyForEntity(ecs::entity entity) {
    auto world = Engine::GetWorld();
    
    if (!world.has_all_components_in_entity<RigidBodyComponent, TransformComponent>(entity) || !world.has_any_components_in_entity<BoxCollider,SphereCollider,CapsuleCollider>(entity)) {
        return;  // Needs to have rigid and transform + one of the 3 colliders
    }

    auto [rb, transform] = world.get_component_from_entity<RigidBodyComponent, TransformComponent>(entity);

    // Don't create duplicate
    if (m_entityToBodyID.contains(entity)) {
        spdlog::warn("PhysicsSystem: Body already exists for entity {}", entity.get_uuid());
        return;
    }

    JPH::RefConst<JPH::Shape> shape = CreateShapeFromCollider(entity);
    if (!shape) {
        spdlog::error("PhysicsSystem: Failed to create shape for entity {}", entity.get_uuid());
        return;
    }

    // Get friction/restitution from collider
    float friction = 0.5f;
    float restitution = 0.0f;
    
    if (world.has_all_components_in_entity<BoxCollider>(entity)) {
        friction = entity.get<BoxCollider>().friction;
        restitution = entity.get<BoxCollider>().restitution;
    }
    else if (world.has_all_components_in_entity<SphereCollider>(entity)) {
        friction = entity.get<SphereCollider>().friction;
        restitution = entity.get<SphereCollider>().restitution;
    }
    else if (world.has_all_components_in_entity<CapsuleCollider>(entity)) {
        friction = entity.get<CapsuleCollider>().friction;
        restitution = entity.get<CapsuleCollider>().restitution;
    }

    // Create body
    JPH::BodyCreationSettings bodySettings(
        shape,
        PhysicsUtils::ToJolt(transform.m_Translation),
        PhysicsUtils::EulerDegreesToJoltQuat(transform.m_Rotation),
        rb.ToJoltMotionType(),
        rb.IsStatic() ? Layers::NON_MOVING : Layers::MOVING
    );

    bodySettings.mFriction = friction;
    bodySettings.mRestitution = restitution;

    JPH::BodyID bodyID = m_bodyInterface->CreateAndAddBody(bodySettings, rb.isActive ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
    if (bodyID.IsInvalid()) {
        spdlog::warn("PhysicsSystem: Body Id is invalid");
        return;
    }


    // Store all mappings
    m_entityToBodyID[entity] = bodyID;
    m_bodyIDToEntity[bodyID.GetIndexAndSequenceNumber()] = entity;
    //auto pos1 = m_bodyInterface->GetPosition(bodyID);
    //m_bodyInterface->SetPosition(bodyID, { 3,0,0 }, JPH::EActivation::Activate);
    //auto pos2 = m_bodyInterface->GetPosition(bodyID);

    spdlog::critical("PhysicsSystem: Created body {} for entity {}", bodyID.GetIndexAndSequenceNumber(), entity.get_uuid());
    spdlog::warn("Trying to get back Entity has {}, BodyID has {}", m_entityToBodyID[entity].GetIndexAndSequenceNumber(), m_bodyIDToEntity[bodyID.GetIndexAndSequenceNumber()].get_uuid());

}

ecs::entity PhysicsSystem::GetEntityFromBodyID(JPH::BodyID bodyID) const {
    if (bodyID.IsInvalid()) return ecs::entity();

    auto it = m_bodyIDToEntity.find(bodyID.GetIndexAndSequenceNumber());
    if (it == m_bodyIDToEntity.end()) return ecs::entity();

    // Convert UID back to entity (you'll need world reference)
    auto world = Engine::GetWorld();  // Need to store world reference somewhere
    // Iterate entities to find matching UID (or use a better lookup if available)
    auto entities = world.get_all_entities();
    for (auto& e : entities) {
        if (e == it->second) {
            spdlog::info("Found Entity {} paired to {}", e.get_uuid(), (*it).first);
            return e;
        }
    }
    return ecs::entity();
}

JPH::BodyID PhysicsSystem::GetBodyID(ecs::entity entity) const {
    uint64_t uid = entity.get_uuid();
    auto it = m_entityToBodyID.find(uid);
    return (it != m_entityToBodyID.end()) ? it->second : JPH::BodyID();
}

bool PhysicsSystem::IsEntityTrigger(ecs::entity entity) {
    auto world = Engine::GetWorld();
    // Check all collider types
    if (world.has_all_components_in_entity<BoxCollider>(entity)) {
        return entity.get<BoxCollider>().isTrigger;
    }
    if (world.has_all_components_in_entity<SphereCollider>(entity)) {
        return entity.get<SphereCollider>().isTrigger;
    }
    if (world.has_all_components_in_entity<CapsuleCollider>(entity)) {
        return entity.get<CapsuleCollider>().isTrigger;
    }
    return false;
}

CollisionInfo PhysicsSystem::BuildCollisionInfo(ecs::entity otherEntity, const JPH::Body& otherBody, const JPH::ContactManifold& manifold, bool flipNormal) {
    CollisionInfo info;
    info.otherEntity = otherEntity;
    info.otherBodyID = otherBody.GetID();

    // Contact point (use first contact point in manifold)
    if (manifold.mRelativeContactPointsOn1.size() > 0) {
        JPH::Vec3 contactPointLocal = manifold.mRelativeContactPointsOn1[0];
        JPH::Vec3 contactPointWorld = manifold.mBaseOffset + contactPointLocal;
        info.contactPoint = PhysicsUtils::ToGLM(contactPointWorld);
    }

    // Contact normal (pointing from body2 to body1 by default)
    JPH::Vec3 normal = manifold.mWorldSpaceNormal;
    if (flipNormal) normal = -normal;  // Flip for second entity
    info.contactNormal = PhysicsUtils::ToGLM(normal);

    // Penetration depth
    info.penetrationDepth = manifold.mPenetrationDepth;

    // Relative velocity (approximate from body velocities)
    JPH::Vec3 vel1 = otherBody.GetLinearVelocity();
    JPH::Vec3 vel2 = JPH::Vec3::sZero();  // Would need to get from other body
    info.relativeVelocity = PhysicsUtils::ToGLM(vel1 - vel2);

    return info;
}

TriggerInfo PhysicsSystem::BuildTriggerInfo(ecs::entity otherEntity, JPH::BodyID otherBodyID) {
    TriggerInfo info;
    info.otherEntity = otherEntity;
    info.otherBodyID = otherBodyID;
    return info;
}

template<typename EventType>
void PhysicsSystem::InvokeColliderCallback(ecs::entity entity, const EventType& eventData, const char* callbackType) {
    auto world = Engine::GetWorld();
    // Try BoxCollider
    if (world.has_all_components_in_entity<BoxCollider>(entity)) {
        auto& collider = entity.get<BoxCollider>();

        if constexpr (std::is_same_v<EventType, CollisionInfo>) {
            if (strcmp(callbackType, "Enter") == 0 && collider.onCollisionEnter) {
                collider.onCollisionEnter(eventData);
            }
            else if (strcmp(callbackType, "Stay") == 0 && collider.onCollisionStay) {
                collider.onCollisionStay(eventData);
            }
        }
        else if constexpr (std::is_same_v<EventType, ecs::entity>) {
            if (strcmp(callbackType, "Exit") == 0 && collider.onCollisionExit) {
                collider.onCollisionExit(eventData);
            }
        }
        else if constexpr (std::is_same_v<EventType, TriggerInfo>) {
            if (strcmp(callbackType, "TriggerEnter") == 0 && collider.onTriggerEnter) {
                collider.onTriggerEnter(eventData);
            }
            else if (strcmp(callbackType, "TriggerStay") == 0 && collider.onTriggerStay) {
                collider.onTriggerStay(eventData);
            }
        }
        return;
    }

    // Try SphereCollider
    if (world.has_all_components_in_entity<SphereCollider>(entity)) {
        auto& collider = entity.get<SphereCollider>();

        if constexpr (std::is_same_v<EventType, CollisionInfo>) {
            if (strcmp(callbackType, "Enter") == 0 && collider.onCollisionEnter) {
                collider.onCollisionEnter(eventData);
            }
            else if (strcmp(callbackType, "Stay") == 0 && collider.onCollisionStay) {
                collider.onCollisionStay(eventData);
            }
        }
        else if constexpr (std::is_same_v<EventType, ecs::entity>) {
            if (strcmp(callbackType, "Exit") == 0 && collider.onCollisionExit) {
                collider.onCollisionExit(eventData);
            }
        }
        else if constexpr (std::is_same_v<EventType, TriggerInfo>) {
            if (strcmp(callbackType, "TriggerEnter") == 0 && collider.onTriggerEnter) {
                collider.onTriggerEnter(eventData);
            }
            else if (strcmp(callbackType, "TriggerStay") == 0 && collider.onTriggerStay) {
                collider.onTriggerStay(eventData);
            }
        }
        return;
    }

    // Try CapsuleCollider
    if (world.has_all_components_in_entity<CapsuleCollider>(entity)) {
        auto& collider = entity.get<CapsuleCollider>();

        if constexpr (std::is_same_v<EventType, CollisionInfo>) {
            if (strcmp(callbackType, "Enter") == 0 && collider.onCollisionEnter) {
                collider.onCollisionEnter(eventData);
            }
            else if (strcmp(callbackType, "Stay") == 0 && collider.onCollisionStay) {
                collider.onCollisionStay(eventData);
            }
        }
        else if constexpr (std::is_same_v<EventType, ecs::entity>) {
            if (strcmp(callbackType, "Exit") == 0 && collider.onCollisionExit) {
                collider.onCollisionExit(eventData);
            }
        }
        else if constexpr (std::is_same_v<EventType, TriggerInfo>) {
            if (strcmp(callbackType, "TriggerEnter") == 0 && collider.onTriggerEnter) {
                collider.onTriggerEnter(eventData);
            }
            else if (strcmp(callbackType, "TriggerStay") == 0 && collider.onTriggerStay) {
                collider.onTriggerStay(eventData);
            }
        }
        return;
    }
}

void PhysicsSystem::HandleCollisionEnter(
    const JPH::Body& body1,
    const JPH::Body& body2,
    const JPH::ContactManifold& manifold)
{
    ecs::entity entity1 = GetEntityFromBodyID(body1.GetID());
    ecs::entity entity2 = GetEntityFromBodyID(body2.GetID());

    if (!entity1 || !entity2) return;

    // Check if either is a trigger
    bool trigger1 = IsEntityTrigger(entity1);
    bool trigger2 = IsEntityTrigger(entity2);

    if (trigger1 || trigger2) {
        // Handle as trigger event
        TriggerInfo info1 = BuildTriggerInfo(entity2, body2.GetID());
        TriggerInfo info2 = BuildTriggerInfo(entity1, body1.GetID());

        InvokeColliderCallback(entity1, info1, "TriggerEnter");
        InvokeColliderCallback(entity2, info2, "TriggerEnter");
    }
    else {
        // Handle as solid collision
        CollisionInfo info1 = BuildCollisionInfo(entity2, body2, manifold, false);
        CollisionInfo info2 = BuildCollisionInfo(entity1, body1, manifold, true);

        InvokeColliderCallback(entity1, info1, "Enter");
        InvokeColliderCallback(entity2, info2, "Enter");
    }
}

void PhysicsSystem::HandleCollisionStay(
    const JPH::Body& body1,
    const JPH::Body& body2,
    const JPH::ContactManifold& manifold)
{
    ecs::entity entity1 = GetEntityFromBodyID(body1.GetID());
    ecs::entity entity2 = GetEntityFromBodyID(body2.GetID());

    if (!entity1 || !entity2) return;

    bool trigger1 = IsEntityTrigger(entity1);
    bool trigger2 = IsEntityTrigger(entity2);

    if (trigger1 || trigger2) {
        TriggerInfo info1 = BuildTriggerInfo(entity2, body2.GetID());
        TriggerInfo info2 = BuildTriggerInfo(entity1, body1.GetID());

        InvokeColliderCallback(entity1, info1, "TriggerStay");
        InvokeColliderCallback(entity2, info2, "TriggerStay");
    }
    else {
        CollisionInfo info1 = BuildCollisionInfo(entity2, body2, manifold, false);
        CollisionInfo info2 = BuildCollisionInfo(entity1, body1, manifold, true);

        InvokeColliderCallback(entity1, info1, "Stay");
        InvokeColliderCallback(entity2, info2, "Stay");
    }
}

void PhysicsSystem::HandleCollisionExit(const JPH::SubShapeIDPair& subShapePair)
{
    // SubShapeIDPair contains both BodyIDs
    JPH::BodyID bodyID1 = subShapePair.GetBody1ID();
    JPH::BodyID bodyID2 = subShapePair.GetBody2ID();

    ecs::entity entity1 = GetEntityFromBodyID(bodyID1);
    ecs::entity entity2 = GetEntityFromBodyID(bodyID2);

    if (!entity1 || !entity2) return;

    bool trigger1 = IsEntityTrigger(entity1);
    bool trigger2 = IsEntityTrigger(entity2);

    if (trigger1 || trigger2) {
        // Trigger exit uses entity, not TriggerInfo
        InvokeColliderCallback(entity1, entity2, "TriggerExit");
        InvokeColliderCallback(entity2, entity1, "TriggerExit");

        // If they have onTriggerExit callbacks expecting TriggerInfo, adjust accordingly
        // For now matching your signature: void(*)(ecs::entity)
    }
    else {
        // Collision exit also uses entity
        InvokeColliderCallback(entity1, entity2, "Exit");
        InvokeColliderCallback(entity2, entity1, "Exit");
    }
}

JPH::RefConst<JPH::Shape> PhysicsSystem::CreateShapeFromCollider(ecs::entity entity) {
    // Check BoxCollider
    auto world = Engine::GetWorld();

    if (world.has_all_components_in_entity<BoxCollider>(entity)) {
        auto& box = entity.get<BoxCollider>();
        JPH::Vec3 halfExtents = PhysicsUtils::ToJolt(box.size * 0.5f);
        return new JPH::BoxShape(halfExtents);
    }

    // Check SphereCollider
    if (world.has_all_components_in_entity<SphereCollider>(entity)) {
        auto& sphere = entity.get<SphereCollider>();
        return new JPH::SphereShape(sphere.radius);
    }

    // Check CapsuleCollider
    if (world.has_all_components_in_entity<CapsuleCollider>(entity)) {
        auto& capsule = entity.get<CapsuleCollider>();
        return new JPH::CapsuleShape(capsule.GetHeight() * 0.5f, capsule.GetRadius());
    }

    return nullptr;
}

void PhysicsSystem::CreateAllBodiesForLoadedScene() {
    spdlog::info("PhysicsSystem: Creating bodies for all loaded entities...");
    auto world = Engine::GetWorld();
    int createdCount = 0;
    int skippedCount = 0;

    // Query all entities that have RigidBodyComponent + TransformComponent + any collider
    auto entities = world.filter_entities<RigidBodyComponent, TransformComponent>();

    for (auto const& entity : entities) {
        // Check if entity has at least one collider
        if (!world.has_any_components_in_entity<BoxCollider, SphereCollider, CapsuleCollider>(entity)) {
            skippedCount++;
            spdlog::debug("PhysicsSystem: Entity {} has RigidBody but no collider, skipping", entity.get_uuid());
            continue;
        }

        // Check if body already exists (shouldn't happen, but safety check)
        if (m_entityToBodyID.contains(entity)) {
            skippedCount++;
            spdlog::debug("PhysicsSystem: Body already exists for entity {}, skipping", entity.get_uuid());
            continue;
        }

        // Create the body
        TryCreateBodyForEntity(entity);
        createdCount++;
    }

    spdlog::info("PhysicsSystem: Created {} bodies, skipped {} entities", createdCount, skippedCount);
}