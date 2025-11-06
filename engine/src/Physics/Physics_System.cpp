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

    virtual JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1,
        const JPH::Body& inBody2,
        JPH::RVec3Arg inBaseOffset,
        const JPH::CollideShapeResult& inCollisionResult) override {
        // Accept all contacts
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    virtual void OnContactAdded(const JPH::Body& inBody1,
        const JPH::Body& inBody2,
        const JPH::ContactManifold& inManifold,
        JPH::ContactSettings& ioSettings) override {
        // Handle collision enter events
        // You can dispatch events to your ECS here
    }

    virtual void OnContactPersisted(const JPH::Body& inBody1,
        const JPH::Body& inBody2,
        const JPH::ContactManifold& inManifold,
        JPH::ContactSettings& ioSettings) override {
        // Handle collision stay events
    }

    virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override {
        // Handle collision exit events
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
}

JPH::BodyID PhysicsSystem::CreateCharacterController(
    ecs::world& world,
    ecs::entity entity,
    const CharacterControllerComponent& charComp,
    const PositionComponent& pos,
    const RotationComponent& rot)
{
    if (!m_bodyInterface) return JPH::BodyID();

    // Create capsule shape for character
    JPH::Ref<JPH::Shape> capsuleShape = new JPH::CapsuleShape(
        charComp.capsuleHeight * 0.5f,  // Half height
        charComp.capsuleRadius
    );

    // Create character settings
    JPH::Ref<JPH::CharacterSettings> settings = new JPH::CharacterSettings();
    settings->mMaxSlopeAngle = JPH::DegreesToRadians(charComp.maxSlopeAngle);
    settings->mMass = charComp.mass;
    settings->mFriction = charComp.friction;
    settings->mGravityFactor = charComp.gravityFactor;
    settings->mShape = capsuleShape;
    settings->mLayer = Layers::MOVING;

    // Create the character
    JPH::Character* character = new JPH::Character(
        settings,
        PhysicsUtils::ToJolt(pos.m_WorldPos),
        PhysicsUtils::EulerToJoltQuat(rot.m_Rotation),
        entity.get_uuid(),  // Store entity ID as user data
        m_physicsSystem.get()
    );

    // Add character to physics system
    character->AddToPhysicsSystem(JPH::EActivation::Activate);

    JPH::BodyID bodyID = character->GetBodyID();



    return bodyID;
}

void PhysicsSystem::UpdateCharacterControllers(ecs::world& world, float deltaTime) {
    if (!m_bodyInterface) return;
    /*
    auto characterEntities = world.filter_entities<
        CharacterControllerComponent,
        PositionComponent,
        RotationComponent
    >();

    for (auto const& entity : characterEntities) {
        auto [charComp, pos, rot] = entity.get<
            CharacterControllerComponent,
            PositionComponent,
            RotationComponent
        >();

        if (!charComp.character || charComp.bodyID.IsInvalid()) continue;

        JPH::Character* character = charComp.character.GetPtr();

        // Get current velocity
        JPH::Vec3 velocity = character->GetLinearVelocity();

        // Apply horizontal movement input
        JPH::Vec3 horizontalVelocity{};// = charComp.moveSpeed; // PhysicsUtils::ToJolt(charComp.movementInput) * charComp.moveSpeed;

        // Preserve vertical velocity (gravity)
        horizontalVelocity.SetY(velocity.GetY());

        // Check ground state
        JPH::Character::EGroundState groundState = character->GetGroundState();
        charComp.isOnGround = (groundState == JPH::Character::EGroundState::OnGround);

        // Handle jumping
        if (charComp.wantsToJump && charComp.isOnGround) {
            horizontalVelocity.SetY(charComp.jumpSpeed);
            charComp.wantsToJump = false;  // Reset jump flag
        }

        // Apply velocity to character
        character->SetLinearVelocity(horizontalVelocity);

        // Update character physics (important!)
        character->PostSimulation(0.05f);  // Max separation distance

        // Sync position back to ECS
        JPH::Vec3 newPosition = character->GetPosition();
        JPH::Quat newRotation = character->GetRotation();

        pos.m_WorldPos = PhysicsUtils::ToGLM(newPosition);
        rot.m_Rotation = PhysicsUtils::JoltQuatToEuler(newRotation);

        // Cache velocity for external queries
        charComp.velocity = character->GetLinearVelocity();
    }*/
}


void PhysicsSystem::FixedUpdate(ecs::world& world) {
    PF_SYSTEM("PhysicsSystem");
    if (!m_physicsSystem || !m_bodyInterface) return;

    const float deltaTime = 1.0f / 60.0f;

    // 1. Update character controllers BEFORE physics step
    UpdateCharacterControllers(world, deltaTime);

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
    auto list_of_entities = world.filter_entities<RigidBodyComponent, TransformComponent, ScaleComponent, RotationComponent>();

    for (auto const& entity : list_of_entities) 
    {
        auto [RigidBody, Transform] {entity.get<RigidBodyComponent, TransformComponent>()};

        m_bodyInterface->SetPositionAndRotation(RigidBody.bodyID, PhysicsUtils::ToJolt(Transform.m_Translation), PhysicsUtils::EulerToJoltQuat(Transform.m_Rotation), JPH::EActivation::DontActivate);
        //if (RigidBody.motionType == JPH::EMotionType::Kinematic && RigidBody.isActive)
    }
}

void PhysicsSystem::SyncTransformsFromPhysics(ecs::world& world) {
    if (!m_bodyInterface) return;

    // Update ECS transforms from dynamic bodies
    auto list_of_entities = world.filter_entities<RigidBodyComponent, TransformComponent>();

    for (auto const& entity : list_of_entities) 
    {
        auto [RigidBody, Transform] {entity.get<RigidBodyComponent, TransformComponent>()};

        if (RigidBody.motionType == JPH::EMotionType::Dynamic && RigidBody.isActive) {
            JPH::Vec3 position;
            JPH::Quat rotation;
            m_bodyInterface->GetPositionAndRotation(RigidBody.bodyID, position, rotation);

            Transform.m_Translation = PhysicsUtils::ToGLM(position);
            Transform.m_Rotation = PhysicsUtils::JoltQuatToEuler(rotation);

            // Update cached velocity
            RigidBody.velocity = m_bodyInterface->GetLinearVelocity(RigidBody.bodyID);
            RigidBody.angularVelocity = m_bodyInterface->GetAngularVelocity(RigidBody.bodyID);
        }
    }
}

void PhysicsSystem::ProcessCollisionEvents(ecs::world&) {
    // Process any queued collision events
    // This depends on how you want to handle events in your ECS
    // You might dispatch events, set flags on components, etc.
}

//void PhysicsSystem::OnEntityCreated(Entity entity) {
//    auto* transform = m_ecsWorld->GetComponent<TransformComponent>(entity);
//    auto* rigidBody = m_ecsWorld->GetComponent<RigidBodyComponent>(entity);
//    auto* collider = m_ecsWorld->GetComponent<ColliderComponent>(entity);
//
//    if (!transform) return;
//
//    // Case 1: RigidBody + Collider (standard physics object)
//    if (rigidBody && collider) {
//        rigidBody->bodyID = CreateRigidBody(entity, *rigidBody, *transform, collider);
//        rigidBody->hasCollision = true;
//    }
//    // Case 2: RigidBody only (kinematic tracker, force applicator)
//    else if (rigidBody) {
//        rigidBody->bodyID = CreateRigidBody(entity, *rigidBody, *transform, nullptr);
//        rigidBody->hasCollision = false;
//    }
//    // Case 3: Collider only (static collision, no dynamics)
//    // else if (collider) {
//    //     CreateStaticCollider(entity, *collider, *transform);
//    // }
//}

//void PhysicsSystem::OnEntityDestroyed(ecs::entity entity) {
//    auto it = m_entityToBody.find(entity);
//    if (it != m_entityToBody.end()) {
//        DestroyRigidBody(it->second);
//    }
//}

// Creates the body of a specific shape and returns the new bodyID
JPH::BodyID PhysicsSystem::CreateRigidBody(ecs::world& world, ecs::entity entity, const RigidBodyComponent& rbComp, const PositionComponent& Pos, const RotationComponent& rot, const ColliderComponent* collider)
{
    if (!m_bodyInterface) return JPH::BodyID();

    JPH::RefConst<JPH::Shape> shape;

    if (collider) {
        shape = collider->shape;
    }
    else {
        // Create a minimal sensor shape for bodies without collision
        shape = new JPH::SphereShape(0.01f);
    }

    // Create body settings
    JPH::BodyCreationSettings bodySettings(shape, PhysicsUtils::ToJolt(Pos.m_WorldPos), PhysicsUtils::EulerToJoltQuat(rot.m_Rotation), rbComp.motionType, rbComp.motionType == JPH::EMotionType::Static ? Layers::NON_MOVING : Layers::MOVING);

    // Set mass properties
    if (rbComp.motionType == JPH::EMotionType::Dynamic) {
        bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateMassAndInertia;
        bodySettings.mMassPropertiesOverride.mMass = rbComp.mass;
    }

    // Set material properties
    if (collider) {
        bodySettings.mFriction = collider->friction;
        bodySettings.mRestitution = collider->restitution;
    }

    // Create the body
    JPH::Body* body = m_bodyInterface->CreateBody(bodySettings);
    if (!body) {
        // Handle error - log or throw exception
        return JPH::BodyID();
    }

    JPH::BodyID bodyID = body->GetID();

    // Add to physics world
    m_bodyInterface->AddBody(bodyID, JPH::EActivation::Activate);

    // Store mapping
    m_JoltBodyIDs.push_back(bodyID);

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