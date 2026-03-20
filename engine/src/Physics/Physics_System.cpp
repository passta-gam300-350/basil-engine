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
#include <algorithm> // for std::sort in RaycastAll

// Jolt DebugRenderer for accessing sInstance singleton
//#include <Jolt/Renderer/DebugRenderer.h>

// For applying center offset to shapes
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

// JoltDebugRenderer (owned by RenderSystem, accessed via singleton)
#include "Render/JoltDebugRenderer.h"

#include "Render/Render.h"
#include "System/BehaviourSystem.hpp"

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

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine)
{
    // Print to the TTY
    std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << std::endl;

    // Breakpoint
    return true;
};

#endif // JPH_ENABLE_ASSERTS


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
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)


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
    if (!m_physicsSystem || !m_bodyInterface) return;

    const float deltaTime = 1.0f / 60.f;
    // 0. Sync dirty components BEFORE physics update
    {
        PF_SCOPE("SyncDirtyComponents");
        SyncDirtyRigidBodies(world);
        SyncDirtyColliders(world);
    }

    // 5. Draw physics debug visualization (if enabled)
    // Note: JoltDebugRenderer is owned by RenderSystem, accessed via Jolt singleton
    if (JPH::DebugRenderer::sInstance)
    {
        // Check if debug renderer is enabled
        auto* debugRenderer = static_cast<JoltDebugRenderer*>(JPH::DebugRenderer::sInstance);
        if (debugRenderer && debugRenderer->IsEnabled())
        {
            JPH::BodyManager::DrawSettings drawSettings;
            drawSettings.mDrawShape = m_drawShapes;
            drawSettings.mDrawShapeWireframe = m_drawShapes;
            drawSettings.mDrawBoundingBox = m_drawBoundingBoxes;
            drawSettings.mDrawVelocity = m_drawVelocities;
            drawSettings.mDrawGetSupportFunction = false;
            drawSettings.mDrawGetSupportingFace = false;
            drawSettings.mDrawShapeColor = JPH::BodyManager::EShapeColor::MotionTypeColor;

            // Draw all bodies in the physics world using the singleton
            m_physicsSystem->DrawBodies(drawSettings, JPH::DebugRenderer::sInstance);
        }
    }

    // 1. Sync transforms from ECS to physics (for kinematic bodies)
    {
        PF_SCOPE("SyncIntoJolt");
        SyncTransformsToPhysics(world);
    }

    if (!isActive) return;


    // 2. Step the physics simulation
    {
        PF_SCOPE("JoltUpdate");
        const int collisionSteps = 1;
        m_physicsSystem->Update(deltaTime, collisionSteps, m_tempAllocator.get(), m_jobSystem.get());
    }

    // 3. Sync transforms from physics back to ECS (for dynamic bodies)
    {
        PF_SCOPE("SyncIntoComponent");
        SyncTransformsFromPhysics(world);
    }

    // 4. Process collision events
    ProcessCollisionEvents(world);
}

void PhysicsSystem::SyncTransformsToPhysics(ecs::world& world) {
    if (!m_bodyInterface) return;

    // Update kinematic bodies from ECS transforms
    auto list_of_entities = world.filter_entities<RigidBodyComponent, TransformComponent>();

    for (auto const& entity : list_of_entities) 
    {
        if (!world.has_any_components_in_entity<BoxCollider, SphereCollider, CapsuleCollider, MeshCollider>(entity)) { continue; }
        auto [RigidBody, Transform] {entity.get<RigidBodyComponent, TransformComponent>()};

        m_bodyInterface->SetPositionAndRotation(m_entityToBodyID[entity], PhysicsUtils::ToJolt(Transform.m_Translation), PhysicsUtils::EulerDegreesToJoltQuat(Transform.m_Rotation), JPH::EActivation::Activate);
    }
}

struct ColliderFit {
    glm::vec3 extents{ 1.f };
    glm::vec3 offset{ 0.f };
};

std::pair<std::unordered_map<rp::Guid, ResourceHandle>&, std::unordered_map<ResourceHandle, ColliderFit>&> GetMeshFittingCache() {
    static std::unordered_map<rp::Guid, ResourceHandle> guid_to_handle;
    static std::unordered_map<ResourceHandle, ColliderFit> handle_to_fit;
    return { guid_to_handle, handle_to_fit };
}

//very expensive o(n)
AABB ComputeExactAABBTransformed(std::vector<Vertex> const& vert, glm::mat4 const& tfm) {
    glm::vec3 b_min{ std::numeric_limits<float>::max() };
    glm::vec3 b_max{ -std::numeric_limits<float>::max() };
    for (Vertex const& v : vert) {
        glm::vec3 tv = glm::vec3(tfm * glm::vec4(v.Position, 1.f));
        b_min = glm::min(tv, b_min);
        b_max = glm::max(tv, b_max);
    }
    return AABB{ b_min, b_max };
}

ColliderFit ComputeMeshFit(rp::Guid guid, ResourceHandle* hdl = nullptr) {
    rp::BasicIndexedGuid partialndguid{ guid, 0ul };
    partialndguid.m_guid.m_low += 1;
    std::vector<glm::mat4> node_global;
    ColliderFit fit{};
    auto meshes = ResourceRegistry::Instance().Get<std::vector<std::pair<std::string, std::shared_ptr<Mesh>>>>(guid, hdl);
    if (!meshes)
        return fit;
    MeshMetaRuntimeData* nodedataptr{};
    if (ResourceRegistry::Instance().Exists(partialndguid)) {
        MeshMetaRuntimeData& nodedata = *(nodedataptr = ResourceRegistry::Instance().Get<MeshMetaRuntimeData>(partialndguid.m_guid));
        node_global.resize(nodedata.node_local_bind.size());
        for (int node = 0; node < nodedata.node_local_bind.size(); ++node) {
            int parent = nodedata.node_parent[node];
            if (parent < 0) {
                // Root node
                node_global[node] = nodedata.node_local_bind[node];
            }
            else {
                // Use cached parent transform
                node_global[node] = node_global[parent] * nodedata.node_local_bind[node];
            }
        }
    }
    glm::vec3 b_min{ std::numeric_limits<float>::max() };
    glm::vec3 b_max{ -std::numeric_limits<float>::max() };
    std::vector < std::pair < glm::vec3, glm::vec3 >> aabbs;
    if (!node_global.empty() && nodedataptr) {
        int matmeshidx{};
        for (auto [meshname, mesh] : *meshes) {
            // 1. approximate fit (too big, o(1) complexity)
            AABB aabb = mesh->GetAABB().Transform(node_global[nodedataptr->mesh_node_idx[matmeshidx++]]);
            
            // 2. exact fit (very expensive, o(n) complexity)
            //AABB aabb = ComputeExactAABBTransformed(mesh->vertices, node_global[nodedataptr->mesh_node_idx[matmeshidx++]]);

            b_min = glm::min(aabb.min, b_min);
            b_max = glm::max(aabb.max, b_max);
            aabbs.emplace_back(std::pair<glm::vec3, glm::vec3>{ aabb.min, aabb.max });
        }
    }
    else {
        for (auto [meshname, mesh] : *meshes) {
            AABB const& aabb = mesh->GetAABB();
            b_min = glm::min(aabb.min, b_min);
            b_max = glm::max(aabb.max, b_max);
        }
    }
    fit.extents = b_max - b_min;
    fit.offset = (b_max + b_min) / 2.f;
    rp::serialization::yaml_serializer::serialize(aabbs, "testfitaabb.yaml");
    rp::serialization::yaml_serializer::serialize(fit, "testfit.yaml");
    return fit;
}

std::optional<ColliderFit> FindUpdateFitFromCache(rp::Guid guid) {
    auto [g2h, h2f] { GetMeshFittingCache() };
    ResourceHandle mhdl{ ResourceRegistry::Instance().Find<std::vector<std::pair<std::string, std::shared_ptr<Mesh>>>>(guid) };
    if (auto hit{ g2h.find(guid) }; hit != g2h.end()) {
        if (hit->second != mhdl) {
            h2f.erase(hit->second);
            g2h.erase(guid);
            ColliderFit fit = ComputeMeshFit(guid, &mhdl);
            h2f.emplace(mhdl, fit);
            g2h.emplace(guid, mhdl);
        }
    }
    else {
        ColliderFit fit = ComputeMeshFit(guid, &mhdl);
        h2f.emplace(mhdl, fit);
        g2h.emplace(guid, mhdl);
    }
    
    auto hitf = h2f.find(mhdl);
    return hitf != h2f.end() ? std::make_optional(hitf->second) : std::nullopt;
}

ColliderFit GetMeshFittings(ecs::entity ent) {
    ColliderFit fit;
    if (!ent.all<MeshRendererComponent>())
        return fit;
    MeshRendererComponent& mesh_comp = ent.get<MeshRendererComponent>();
    if (!mesh_comp.m_MeshGuid.m_guid)
        return fit;
    auto res = FindUpdateFitFromCache(mesh_comp.m_MeshGuid.m_guid);
    return res ? res.value() : fit;
}

void PhysicsSystem::ResizeEntityPhysics(ecs::entity ent, glm::vec3 new_scale, glm::vec3 old_scale) {
    if (!HasCollider(ent)) { return; }
    auto bid = GetBodyID(ent);
    if (bid.IsInvalid()) { return; }
    if (glm::compMin(old_scale) < FLT_EPSILON) {
        old_scale = glm::vec3(1.f); //prevents inf; //should log
    }
    [](ecs::entity e, glm::vec3 ratio_scl) {
        if (e.all<BoxCollider>()) {
            e.get<BoxCollider>().size *= ratio_scl;
        }
        if (e.all<SphereCollider>()) {
            auto& sph = e.get<SphereCollider>();
            sph.radius *= std::max({ ratio_scl.x, ratio_scl.y, ratio_scl.z });
        }
        if (e.all<CapsuleCollider>()) {
            auto& cap = e.get<CapsuleCollider>();
            float sclfx = std::max({ ratio_scl.x, ratio_scl.z });
            cap.SetRadius(0.5f * sclfx);
            cap.SetHeight(1.f * ratio_scl.y);
        }
        if (e.all<MeshCollider>()) {
            //e.get<MeshCollider>(); //not supported
        }
        } (ent, new_scale/old_scale);
    m_bodyInterface->SetShape(bid, CreateShapeFromCollider(ent), true, JPH::EActivation::Activate);
}

void PhysicsSystem::SyncEntityTransformsToPhysics(ecs::entity ent) {
    if (!m_bodyInterface) return;
    auto bid = GetBodyID(ent);
    if (!bid.IsInvalid() && ent.all<TransformComponent>())
    {
        auto Transform {ent.get<TransformComponent>()};
        m_bodyInterface->SetPositionAndRotation(bid, PhysicsUtils::ToJolt(Transform.m_Translation), PhysicsUtils::EulerDegreesToJoltQuat(Transform.m_Rotation), JPH::EActivation::Activate);
    }
}

void PhysicsSystem::SyncTransformsFromPhysics(ecs::world& world) {
    if (!m_bodyInterface) return;

    // Update ECS transforms from dynamic bodies
    auto list_of_entities = world.filter_entities<RigidBodyComponent, TransformComponent>();

    for (auto const& entity : list_of_entities)
    {
        if (!world.has_any_components_in_entity<BoxCollider, SphereCollider, CapsuleCollider, MeshCollider>(entity)) { continue; }
        auto [RigidBody, Transform] {entity.get<RigidBodyComponent, TransformComponent>()};

        if ((RigidBody.motionType == RigidBodyComponent::MotionType::Dynamic) && RigidBody.isActive) {
            JPH::BodyID BodyId = m_entityToBodyID[entity];

            // Store original constrained values
            glm::vec3 originalPosition = Transform.m_Translation;
            glm::vec3 originalRotation = Transform.m_Rotation;

            JPH::Vec3 position;
            JPH::Quat rotation;
            m_bodyInterface->GetPositionAndRotation(BodyId, position, rotation);

            glm::vec3 newPosition = PhysicsUtils::ToGLM(position);
            glm::vec3 newRotation = PhysicsUtils::JoltQuatToEulerDegrees(rotation);

            // Apply position constraints - restore frozen axes
            if (RigidBody.freezePositionX) newPosition.x = originalPosition.x;
            if (RigidBody.freezePositionY) newPosition.y = originalPosition.y;
            if (RigidBody.freezePositionZ) newPosition.z = originalPosition.z;

            // Apply rotation constraints - restore frozen axes
            if (RigidBody.freezeRotationX) newRotation.x = originalRotation.x;
            if (RigidBody.freezeRotationY) newRotation.y = originalRotation.y;
            if (RigidBody.freezeRotationZ) newRotation.z = originalRotation.z;

            Transform.m_Translation = newPosition;
            Transform.m_Rotation = newRotation;

            // Update velocities from Jolt
            glm::vec3 linearVel = PhysicsUtils::ToGLM(m_bodyInterface->GetLinearVelocity(BodyId));
            glm::vec3 angularVel = PhysicsUtils::ToGLM(m_bodyInterface->GetAngularVelocity(BodyId));

            // IMPORTANT: Frozen position/rotation axes must also freeze corresponding velocities
            // This prevents velocity accumulation on frozen axes and ensures forces don't build up

            // If position is frozen on an axis, velocity on that axis must be zero
            if (RigidBody.freezePositionX) linearVel.x = 0.0f;
            if (RigidBody.freezePositionY) linearVel.y = 0.0f;
            if (RigidBody.freezePositionZ) linearVel.z = 0.0f;

            // If rotation is frozen on an axis, angular velocity on that axis must be zero
            if (RigidBody.freezeRotationX) angularVel.x = 0.0f;
            if (RigidBody.freezeRotationY) angularVel.y = 0.0f;
            if (RigidBody.freezeRotationZ) angularVel.z = 0.0f;

            // Additional velocity constraints (independent of position/rotation)
            if (RigidBody.freezeLinearVelocityX) linearVel.x = 0.0f;
            if (RigidBody.freezeLinearVelocityY) linearVel.y = 0.0f;
            if (RigidBody.freezeLinearVelocityZ) linearVel.z = 0.0f;

            if (RigidBody.freezeAngularVelocityX) angularVel.x = 0.0f;
            if (RigidBody.freezeAngularVelocityY) angularVel.y = 0.0f;
            if (RigidBody.freezeAngularVelocityZ) angularVel.z = 0.0f;

            // Update cached velocities
            RigidBody.linearVelocity = linearVel;
            RigidBody.angularVelocity = angularVel;

            // Write constrained velocities back to Jolt to prevent accumulation
            // This is critical: we must write back to prevent forces from building up on frozen axes
            bool hasLinearConstraint = RigidBody.freezePositionX || RigidBody.freezePositionY || RigidBody.freezePositionZ ||
                                       RigidBody.freezeLinearVelocityX || RigidBody.freezeLinearVelocityY || RigidBody.freezeLinearVelocityZ;
            bool hasAngularConstraint = RigidBody.freezeRotationX || RigidBody.freezeRotationY || RigidBody.freezeRotationZ ||
                                        RigidBody.freezeAngularVelocityX || RigidBody.freezeAngularVelocityY || RigidBody.freezeAngularVelocityZ;

            if (hasLinearConstraint) {
                m_bodyInterface->SetLinearVelocity(BodyId, PhysicsUtils::ToJolt(linearVel));
            }
            if (hasAngularConstraint) {
                m_bodyInterface->SetAngularVelocity(BodyId, PhysicsUtils::ToJolt(angularVel));
            }

            // If position/rotation was constrained, write it back to Jolt to enforce constraints
            if (RigidBody.freezePositionX || RigidBody.freezePositionY || RigidBody.freezePositionZ ||
                RigidBody.freezeRotationX || RigidBody.freezeRotationY || RigidBody.freezeRotationZ) {
                m_bodyInterface->SetPositionAndRotation(BodyId,
                    PhysicsUtils::ToJolt(newPosition),
                    PhysicsUtils::EulerDegreesToJoltQuat(newRotation),
                    JPH::EActivation::DontActivate);
            }
        }
    }
}

void PhysicsSystem::ProcessCollisionEvents(ecs::world&) {
    // Process any queued collision events
    // This depends on how you want to handle events in your ECS
    // You might dispatch events, set flags on components, etc.

    
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
    registry.on_construct<MeshCollider>().connect<&PhysicsSystem::OnColliderAdded>(this);
    registry.on_destroy<MeshCollider>().connect<&PhysicsSystem::OnColliderDestroyed>(this);

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
    registry.on_construct<MeshCollider>().disconnect();
    registry.on_destroy<MeshCollider>().disconnect();

    m_observersEnabled = false;
    spdlog::debug("PhysicsSystem: Observers disabled");
}

void PhysicsSystem::OnRigidbodyAdded(entt::registry& registry, entt::entity entity) {
    // Convert to ecs::entity
    ecs::entity const ecsEntity = Engine::GetWorld().impl.entity_cast(entity);
    auto world = Engine::GetWorld();
    // Only create body if entity also has ColliderComponent
    if (world.has_any_components_in_entity<BoxCollider, CapsuleCollider, SphereCollider>(ecsEntity))
    {
        TryCreateBodyForEntity(ecsEntity);
    }
}

void PhysicsSystem::OnColliderAdded(entt::registry& registry, entt::entity entity) {

    ecs::entity const ecsEntity = Engine::GetWorld().impl.entity_cast(entity);
    auto world = Engine::GetWorld();

    // Create body if entity has RigidBodyComponent OR if it's a trigger-only collider
    // TryCreateBodyForEntity will handle both cases appropriately
    TryCreateBodyForEntity(ecsEntity);
}

void PhysicsSystem::OnRigidbodyDestroyed(entt::registry& registry, entt::entity entity) {
    ecs::entity const ecsEntity = Engine::GetWorld().impl.entity_cast(entity);
    auto world = Engine::GetWorld();
    
    // Find and destroy associated Jolt body
    auto it = m_entityToBodyID.find(ecsEntity);
    if (it != m_entityToBodyID.end()) {
       DestroyRigidBody(it->second);

        m_bodyIDToEntity.erase(it->second.GetIndexAndSequenceNumber());
        m_entityToBodyID.erase(it);
    }

    // If entity still has a collider, recreate as trigger-only/static body
    if (world.has_any_components_in_entity<BoxCollider, SphereCollider, CapsuleCollider, MeshCollider>(ecsEntity)) {
        TryCreateBodyForEntity(ecsEntity);
    }
}

void PhysicsSystem::OnColliderDestroyed(entt::registry& registry, entt::entity entity) {
    // Collider removed = body needs recreation (or destruction if no other colliders)
    ecs::entity const ecsEntity = Engine::GetWorld().impl.entity_cast(entity);
    auto world = Engine::GetWorld();
    spdlog::critical("enter");
    // Find and destroy associated Jolt body
    auto it = m_entityToBodyID.find(ecsEntity);
    if (it != m_entityToBodyID.end()) {
        JPH::BodyID const bodyID = it->second;
        DestroyRigidBody(bodyID);
        m_entityToBodyID.erase(it);
        m_bodyIDToEntity.erase(bodyID.GetIndexAndSequenceNumber());
    }
}

void PhysicsSystem::TryCreateBodyForEntity(ecs::entity entity) {
    auto world = Engine::GetWorld();

    // Must have transform and at least one collider
    if (!world.has_all_components_in_entity<TransformComponent>(entity) ||
        !world.has_any_components_in_entity<BoxCollider, SphereCollider, CapsuleCollider, MeshCollider>(entity)) {
        return;
    }

    // Don't create duplicate
    if (m_entityToBodyID.contains(entity)) {
        spdlog::warn("PhysicsSystem: Body already exists for entity {}", entity.get_uuid());
        return;
    }

    auto transform = entity.get<TransformComponent>();
    bool hasRigidBody = world.has_all_components_in_entity<RigidBodyComponent>(entity);

    JPH::RefConst<JPH::Shape> shape = CreateShapeFromCollider(entity);
    if (!shape) {
        spdlog::error("PhysicsSystem: Failed to create shape for entity {}", entity.get_uuid());
        return;
    }

    // Get friction/restitution/trigger from collider
    float friction = 0.5f;
    float restitution = 0.0f;
    bool isTrigger = false;
    if (world.has_all_components_in_entity<BoxCollider>(entity)) {
        auto Collider = entity.get<BoxCollider>();
        friction = Collider.friction;
        restitution = Collider.restitution;
        isTrigger = Collider.isTrigger;

        Collider.onCollisionEnter = [entity](CollisionInfo const& other) {
            ecs::entity entityHandle{ entity };
            BehaviourSystem::Instance().OnCollisionCallback(entityHandle, other.otherEntity, BehaviourSystem::CollisionCallback::OnCollisionEnter);
		};
        Collider.onCollisionStay = [entity](CollisionInfo const& other) {
            ecs::entity entityHandle{ entity };
            BehaviourSystem::Instance().OnCollisionCallback(entityHandle, other.otherEntity, BehaviourSystem::CollisionCallback::OnCollisionStay);
        };
        Collider.onCollisionExit = [entity](ecs::entity other) {
            ecs::entity entityHandle{ entity };
            BehaviourSystem::Instance().OnCollisionCallback(entityHandle, other, BehaviourSystem::CollisionCallback::OnCollisionExit);
		};

        Collider.onTriggerEnter = [entity](TriggerInfo const& other) {
            ecs::entity entityHandle{ entity };
            BehaviourSystem::Instance().OnCollisionCallback(entityHandle, other.otherEntity, BehaviourSystem::CollisionCallback::OnTriggerEnter);
            };
        Collider.onTriggerStay = [entity](TriggerInfo const& other) {
            ecs::entity entityHandle{ entity };
            BehaviourSystem::Instance().OnCollisionCallback(entityHandle, other.otherEntity, BehaviourSystem::CollisionCallback::OnTriggerStay);
            };
        Collider.onTriggerExit = [entity](ecs::entity other) {
            ecs::entity entityHandle{ entity };
            BehaviourSystem::Instance().OnCollisionCallback(entityHandle, other, BehaviourSystem::CollisionCallback::OnTriggerExit);
            };
    }
    else if (world.has_all_components_in_entity<SphereCollider>(entity)) {
        auto Collider = entity.get<SphereCollider>();
        friction = Collider.friction;
        restitution = Collider.restitution;
        isTrigger = Collider.isTrigger;

        Collider.onCollisionEnter = [entity](CollisionInfo const& other) {
            ecs::entity entityHandle{ entity };
            BehaviourSystem::Instance().OnCollisionCallback(entityHandle, other.otherEntity, BehaviourSystem::CollisionCallback::OnCollisionEnter);
        };
        Collider.onCollisionStay = [entity](CollisionInfo const& other) {
            ecs::entity entityHandle{ entity };
            BehaviourSystem::Instance().OnCollisionCallback(entityHandle, other.otherEntity, BehaviourSystem::CollisionCallback::OnCollisionStay);
        };
        Collider.onCollisionExit = [entity](ecs::entity other) {
            ecs::entity entityHandle{ entity };
            BehaviourSystem::Instance().OnCollisionCallback(entityHandle, other, BehaviourSystem::CollisionCallback::OnCollisionExit);
        };

        Collider.onTriggerEnter = [entity](TriggerInfo const& other) {
            ecs::entity entityHandle{ entity };
            BehaviourSystem::Instance().OnCollisionCallback(entityHandle, other.otherEntity, BehaviourSystem::CollisionCallback::OnTriggerEnter);
            };
        Collider.onTriggerStay = [entity](TriggerInfo const& other) {
            ecs::entity entityHandle{ entity };
            BehaviourSystem::Instance().OnCollisionCallback(entityHandle, other.otherEntity, BehaviourSystem::CollisionCallback::OnTriggerStay);
            };
        Collider.onTriggerExit = [entity](ecs::entity other) {
            ecs::entity entityHandle{ entity };
            BehaviourSystem::Instance().OnCollisionCallback(entityHandle, other, BehaviourSystem::CollisionCallback::OnTriggerExit);
            };
    }
    else if (world.has_all_components_in_entity<CapsuleCollider>(entity)) {
        auto Collider = entity.get<CapsuleCollider>();
        friction = Collider.friction;
        restitution = Collider.restitution;
        isTrigger = Collider.isTrigger;

        Collider.onCollisionEnter = [entity](CollisionInfo const& other) {
            ecs::entity entityHandle{ entity };
            BehaviourSystem::Instance().OnCollisionCallback(entityHandle, other.otherEntity, BehaviourSystem::CollisionCallback::OnCollisionEnter);
        };
        Collider.onCollisionStay = [entity](CollisionInfo const& other) {
            ecs::entity entityHandle{ entity };
            BehaviourSystem::Instance().OnCollisionCallback(entityHandle, other.otherEntity, BehaviourSystem::CollisionCallback::OnCollisionStay);
        };
        Collider.onCollisionExit = [entity](ecs::entity other) {
            ecs::entity entityHandle{ entity };
            BehaviourSystem::Instance().OnCollisionCallback(entityHandle, other, BehaviourSystem::CollisionCallback::OnCollisionExit);
        };

        Collider.onTriggerEnter = [entity](TriggerInfo const& other) {
            ecs::entity entityHandle{ entity };
            BehaviourSystem::Instance().OnCollisionCallback(entityHandle, other.otherEntity, BehaviourSystem::CollisionCallback::OnTriggerEnter);
            };
        Collider.onTriggerStay = [entity](TriggerInfo const& other) {
            ecs::entity entityHandle{ entity };
            BehaviourSystem::Instance().OnCollisionCallback(entityHandle, other.otherEntity, BehaviourSystem::CollisionCallback::OnTriggerStay);
            };
        Collider.onTriggerExit = [entity](ecs::entity other) {
            ecs::entity entityHandle{ entity };
            BehaviourSystem::Instance().OnCollisionCallback(entityHandle, other, BehaviourSystem::CollisionCallback::OnTriggerExit);
            };
    }

    // Determine motion type and layer
    JPH::EMotionType motionType = JPH::EMotionType::Static;
    JPH::ObjectLayer layer = Layers::NON_MOVING;
    bool isActiveBody = true;

    if (hasRigidBody) {
        auto rb = entity.get<RigidBodyComponent>();
        motionType = rb.ToJoltMotionType();
        layer = rb.IsStatic() ? Layers::NON_MOVING : Layers::MOVING;
        isActiveBody = rb.isActive;
    }

    // Triggers are always static
    if (isTrigger) {
        motionType = JPH::EMotionType::Static;
        layer = Layers::NON_MOVING;
    }

    // Create body
    JPH::BodyCreationSettings bodySettings(
        shape,
        PhysicsUtils::ToJolt(transform.m_Translation),
        PhysicsUtils::EulerDegreesToJoltQuat(transform.m_Rotation),
        motionType,
        layer
    );

    bodySettings.mIsSensor = isTrigger;
    bodySettings.mFriction = friction;
    bodySettings.mRestitution = restitution;

    // Apply RigidBody properties to body creation settings
    if (hasRigidBody) {
        auto rb = entity.get<RigidBodyComponent>();

        // Set initial velocities
        bodySettings.mLinearVelocity = PhysicsUtils::ToJolt(rb.linearVelocity);
        bodySettings.mAngularVelocity = PhysicsUtils::ToJolt(rb.angularVelocity);

        // Override friction from RigidBody if it has one
        if (rb.friction > 0.0f) {
            bodySettings.mFriction = rb.friction;
        }

        // Gravity factor (for dynamic/kinematic bodies)
        if (motionType != JPH::EMotionType::Static) {
            bodySettings.mGravityFactor = rb.useGravity ? rb.gravityFactor : 0.0f;
        }

        // Mass override (will be set after creation for dynamic bodies)
        // Linear and angular damping (will be set after creation)
    }
    
    JPH::BodyID bodyID = m_bodyInterface->CreateAndAddBody(bodySettings, isActiveBody ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
    if (bodyID.IsInvalid()) {
        spdlog::warn("PhysicsSystem: Body Id is invalid");
        return;
    }

    // Apply additional properties that require body interface (for non-static bodies)
    if (hasRigidBody && motionType != JPH::EMotionType::Static) {
        auto rb = entity.get<RigidBodyComponent>();

        // Lock body to set additional properties
        JPH::BodyLockWrite lock(m_physicsSystem->GetBodyLockInterface(), bodyID);
        if (lock.Succeeded()) {
            JPH::Body& body = lock.GetBody();
            JPH::MotionProperties* motionProps = body.GetMotionProperties();

            if (motionProps) {
                // Set mass (only for dynamic bodies)
                if (motionType == JPH::EMotionType::Dynamic && rb.mass > 0.0f) {
                    JPH::MassProperties massProps = body.GetShape()->GetMassProperties();
                    massProps.ScaleToMass(rb.mass);
                    motionProps->SetMassProperties(JPH::EAllowedDOFs::All, massProps);
                }

                // Set damping
                motionProps->SetLinearDamping(rb.linearDamping);
                motionProps->SetAngularDamping(rb.angularDrag);

                // Gravity factor already set in creation settings, but ensure it's correct
                motionProps->SetGravityFactor(rb.useGravity ? rb.gravityFactor : 0.0f);
            }
        } else {
            spdlog::warn("PhysicsSystem: Failed to lock body {} for initial property setup",
                bodyID.GetIndexAndSequenceNumber());
        }
    }

    // Store all mappings
    m_entityToBodyID[entity] = bodyID;
    m_bodyIDToEntity[bodyID.GetIndexAndSequenceNumber()] = entity;

    // Set entity reference in RigidBody if it exists
    if (hasRigidBody) {
        auto& rb = entity.get<RigidBodyComponent>();
        rb.m_entity = entity;
    }

    spdlog::info("PhysicsSystem: Created {} body {} for entity {}",
        isTrigger ? "trigger" : (hasRigidBody ? "rigidbody" : "static collider"),
        bodyID.GetIndexAndSequenceNumber(),
        entity.get_uuid());
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
            //spdlog::info("Found Entity {} paired to {}", e.get_uuid(), (*it).first);
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

// Helper function to update collision tracking state
static void UpdateCollisionTracking(ecs::entity entity, ecs::entity otherEntity, bool isEntering) {
    auto world = Engine::GetWorld();

    // Lambda to update collider state
    auto updateCollider = [&](auto& collider) {
        if (isEntering) {
            // Add to collision list if not already present
            auto it = std::find(collider.collidingWith.begin(), collider.collidingWith.end(), otherEntity);
            if (it == collider.collidingWith.end()) {
                collider.collidingWith.push_back(otherEntity);
            }
        } else {
            // Remove from collision list
            auto it = std::find(collider.collidingWith.begin(), collider.collidingWith.end(), otherEntity);
            if (it != collider.collidingWith.end()) {
                collider.collidingWith.erase(it);
            }
        }

        // Update collision count and flag
        collider.collisionCount = static_cast<int>(collider.collidingWith.size());
        collider.isColliding = (collider.collisionCount > 0);
    };

    // Check each collider type and update
    if (world.has_all_components_in_entity<BoxCollider>(entity)) {
        auto& collider = entity.get<BoxCollider>();
        updateCollider(collider);
    }
    if (world.has_all_components_in_entity<SphereCollider>(entity)) {
        auto& collider = entity.get<SphereCollider>();
        updateCollider(collider);
    }
    if (world.has_all_components_in_entity<CapsuleCollider>(entity)) {
        auto& collider = entity.get<CapsuleCollider>();
        updateCollider(collider);
    }
    if (world.has_all_components_in_entity<MeshCollider>(entity)) {
        auto& collider = entity.get<MeshCollider>();
        updateCollider(collider);
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

    // Update collision tracking
    UpdateCollisionTracking(entity1, entity2, true);
    UpdateCollisionTracking(entity2, entity1, true);

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

    // Update collision tracking
    UpdateCollisionTracking(entity1, entity2, false);
    UpdateCollisionTracking(entity2, entity1, false);

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

void PhysicsSystem::PrintDebugInfo(ecs::entity entity) {
    auto world = Engine::GetWorld();
    auto [rb, coll] = world.get_component_from_entity<RigidBodyComponent, BoxCollider>(entity);
    //m_bodyInterface->GetTransformedShape().
    spdlog::critical("entity: {}, ", entity.name());
    
}

JPH::RefConst<JPH::Shape> PhysicsSystem::CreateShapeFromCollider(ecs::entity entity) {
    // Check BoxCollider
    auto world = Engine::GetWorld();

    if (world.has_all_components_in_entity<BoxCollider>(entity)) {
        auto& box = entity.get<BoxCollider>();
        JPH::Vec3 halfExtents = PhysicsUtils::ToJolt(box.size * 0.5f);
        JPH::RefConst<JPH::Shape> baseShape = new JPH::BoxShape(halfExtents, 0.0001f);

        // Apply center offset if non-zero
        JPH::Vec3 centerOffset = PhysicsUtils::ToJolt(box.center);
        if (!centerOffset.IsNearZero()) {
            return new JPH::RotatedTranslatedShape(centerOffset, JPH::Quat::sIdentity(), baseShape);
        }
        return baseShape;
    }

    // Check SphereCollider
    if (world.has_all_components_in_entity<SphereCollider>(entity)) {
        auto& sphere = entity.get<SphereCollider>();
        JPH::RefConst<JPH::Shape> baseShape = new JPH::SphereShape(sphere.radius);

        // Apply center offset if non-zero
        JPH::Vec3 centerOffset = PhysicsUtils::ToJolt(sphere.center);
        if (!centerOffset.IsNearZero()) {
            return new JPH::RotatedTranslatedShape(centerOffset, JPH::Quat::sIdentity(), baseShape);
        }
        return baseShape;
    }

    // Check CapsuleCollider
    if (world.has_all_components_in_entity<CapsuleCollider>(entity)) {
        auto& capsule = entity.get<CapsuleCollider>();
        JPH::RefConst<JPH::Shape> baseShape = new JPH::CapsuleShape(capsule.GetHeight() * 0.5f, capsule.GetRadius());

        // Apply center offset if non-zero
        JPH::Vec3 centerOffset = PhysicsUtils::ToJolt(capsule.center);
        if (!centerOffset.IsNearZero()) {
            return new JPH::RotatedTranslatedShape(centerOffset, JPH::Quat::sIdentity(), baseShape);
        }
        return baseShape;
    }

    // Check MeshCollider
    if (world.has_all_components_in_entity<MeshCollider>(entity)) {
        auto& meshCollider = entity.get<MeshCollider>();

        std::vector<glm::vec3> vertices;
        std::vector<uint32_t> indices;

        // Get mesh data - either from renderer or custom data
        if (meshCollider.useRendererMesh && world.has_all_components_in_entity<MeshRendererComponent>(entity)) {
            // Extract mesh data from MeshRendererComponent
            [[maybe_unused]] auto& meshRenderer = entity.get<MeshRendererComponent>();
            
            // TODO: You need to implement mesh data extraction based on your asset system
            // This is a placeholder - you'll need to load the actual mesh data from your mesh GUID
            // Example approach:
            // auto meshAsset = AssetManager::GetMesh(meshRenderer.m_MeshGuid);
            // vertices = meshAsset->GetVertices();
            // indices = meshAsset->GetIndices();

            spdlog::warn("PhysicsSystem: MeshCollider mesh extraction not fully implemented - using placeholder");

            // For now, return nullptr and log a warning
            // You should implement mesh loading from your asset system
            return nullptr;
        }
        else if (!meshCollider.customVertices.empty() && !meshCollider.customIndices.empty()) {
            // Use manually provided mesh data
            vertices = meshCollider.customVertices;
            indices = meshCollider.customIndices;
        }
        else {
            spdlog::error("PhysicsSystem: MeshCollider has no valid mesh data");
            return nullptr;
        }

        // Create the appropriate shape based on collision mode
        JPH::RefConst<JPH::Shape> baseShape = nullptr;

        if (meshCollider.collisionMode == MeshCollider::MESH) {
            // Create exact mesh shape (static/kinematic only)
            JPH::TriangleList triangles;
            triangles.reserve(indices.size() / 3);

            for (size_t i = 0; i < indices.size(); i += 3) {
                JPH::Vec3 v0 = PhysicsUtils::ToJolt(vertices[indices[i]]);
                JPH::Vec3 v1 = PhysicsUtils::ToJolt(vertices[indices[i + 1]]);
                JPH::Vec3 v2 = PhysicsUtils::ToJolt(vertices[indices[i + 2]]);
                triangles.push_back(JPH::Triangle(v0, v1, v2));
            }

            JPH::MeshShapeSettings settings(triangles);
            JPH::ShapeSettings::ShapeResult result = settings.Create();

            if (result.IsValid()) {
                spdlog::info("PhysicsSystem: Created MeshShape with {} triangles", triangles.size());
                baseShape = result.Get();
            }
            else {
                spdlog::error("PhysicsSystem: Failed to create MeshShape: {}", result.GetError().c_str());
                return nullptr;
            }
        }
        else { // CONVEX_HULL
            // Create convex hull shape (works for dynamic objects)
            std::vector<JPH::Vec3> joltVertices;
            joltVertices.reserve(vertices.size());

            for (const auto& vertex : vertices) {
                joltVertices.push_back(PhysicsUtils::ToJolt(vertex));
            }

            JPH::ConvexHullShapeSettings settings(joltVertices.data(), (int)joltVertices.size(), meshCollider.convexRadius);
            JPH::ShapeSettings::ShapeResult result = settings.Create();

            if (result.IsValid()) {
                spdlog::info("PhysicsSystem: Created ConvexHullShape with {} vertices", joltVertices.size());
                baseShape = result.Get();
            }
            else {
                spdlog::error("PhysicsSystem: Failed to create ConvexHullShape: {}", result.GetError().c_str());
                return nullptr;
            }
        }

        // Apply center offset if non-zero
        JPH::Vec3 centerOffset = PhysicsUtils::ToJolt(meshCollider.center);
        if (!centerOffset.IsNearZero()) {
            return new JPH::RotatedTranslatedShape(centerOffset, JPH::Quat::sIdentity(), baseShape);
        }
        return baseShape;
    }

    return nullptr;
}

void PhysicsSystem::CreateAllBodiesForLoadedScene() {
    spdlog::info("PhysicsSystem: Creating bodies for all loaded entities...");
    auto world = Engine::GetWorld();
    int createdCount = 0;
    int skippedCount = 0;

    // First, create bodies for entities with RigidBodyComponent + collider
    auto rbEntities = world.filter_entities<RigidBodyComponent, TransformComponent>();
    for (auto const& entity : rbEntities) {
        if (!world.has_any_components_in_entity<BoxCollider, SphereCollider, CapsuleCollider>(entity)) {
            skippedCount++;
            spdlog::debug("PhysicsSystem: Entity {} has RigidBody but no collider, skipping", entity.get_uuid());
            continue;
        }

        if (m_entityToBodyID.contains(entity)) {
            skippedCount++;
            spdlog::debug("PhysicsSystem: Body already exists for entity {}, skipping", entity.get_uuid());
            continue;
        }

        TryCreateBodyForEntity(entity);
        createdCount++;
    }

    // Second, create bodies for trigger-only colliders (no RigidBodyComponent)
    auto allEntities = world.get_all_entities();
    for (auto const& entity : allEntities) {
        // Skip if already has RigidBody (handled above)
        if (world.has_all_components_in_entity<RigidBodyComponent>(entity)) {
            continue;
        }

        // Must have transform and at least one collider
        if (!world.has_all_components_in_entity<TransformComponent>(entity)) {
            continue;
        }

        if (!world.has_any_components_in_entity<BoxCollider, SphereCollider, CapsuleCollider>(entity)) {
            continue;
        }

        // Check if body already exists
        if (m_entityToBodyID.contains(entity)) {
            skippedCount++;
            continue;
        }

        // Create the body (will be static or trigger)
        TryCreateBodyForEntity(entity);
        createdCount++;
    }

    spdlog::info("PhysicsSystem: Created {} bodies, skipped {} entities", createdCount, skippedCount);
}

void PhysicsSystem::DestroyAllBodiesForUnload()
{
    spdlog::info("PhysicsSystem: Destroying all bodies for all loaded entities...");
    auto world = Engine::GetWorld();

    if (!m_bodyInterface) {
        spdlog::warn("PhysicsSystem: Body interface not initialized, skipping body destruction");
        return;
    }

    auto entities = world.filter_entities<RigidBodyComponent, TransformComponent>();

    int destroyedCount = 0;
    int missingCount = 0;

    for (auto const& entity : entities)
    {
        auto it = m_entityToBodyID.find(entity);
        if (it == m_entityToBodyID.end()) {
            ++missingCount;
            continue;
        }

        JPH::BodyID bodyID = it->second;

        if (!bodyID.IsInvalid()) {
            m_bodyInterface->RemoveBody(bodyID);
            m_bodyInterface->DestroyBody(bodyID);
            m_bodyIDToEntity.erase(bodyID.GetIndexAndSequenceNumber());
            ++destroyedCount;
        }

        m_entityToBodyID.erase(it);
    }

    spdlog::info("PhysicsSystem: Destroyed {} bodies ({} missing) during unload", destroyedCount, missingCount);
}

void PhysicsSystem::Reset()
{
    spdlog::info("PhysicsSystem: Resetting physics system - destroying all bodies and clearing mappings");

    if (!m_bodyInterface) {
        spdlog::warn("PhysicsSystem: Body interface not initialized, skipping reset");
        return;
    }

    int destroyedCount = 0;

    // Destroy all bodies using the m_entityToBodyID map
    for (auto& [entity, bodyID] : m_entityToBodyID) {
        if (!bodyID.IsInvalid()) {
            m_bodyInterface->RemoveBody(bodyID);
            m_bodyInterface->DestroyBody(bodyID);
            ++destroyedCount;
        }
    }

    // Clear all mapping data structures
    m_entityToBodyID.clear();
    m_bodyIDToEntity.clear();
    m_JoltBodyIDs.clear();

    spdlog::info("PhysicsSystem: Reset complete - destroyed {} bodies and cleared all mappings", destroyedCount);
}


void PhysicsSystem::SyncDirtyRigidBodies(ecs::world& world) {
    // Query all entities with RigidBodyComponent
    auto entities = world.filter_entities<RigidBodyComponent>();

    for (auto const& entity : entities) {
        auto& rb = entity.get<RigidBodyComponent>();

        // Skip if not dirty
        if (!rb.isDirty) continue;

        // Get BodyID
        auto it = m_entityToBodyID.find(entity);
        if (it == m_entityToBodyID.end()) {
            // Body doesn't exist yet (maybe missing collider)
            spdlog::debug("PhysicsSystem: RigidBody dirty but no body exists for entity {}",
                entity.get_uuid());
            rb.isDirty = false;
            continue;
        }

        JPH::BodyID bodyID = it->second;
        if (bodyID.IsInvalid()) {
            rb.isDirty = false;
            continue;
        }

        // Update body properties
        UpdateBodyProperties(bodyID, rb);

        // Clear dirty flag
        rb.isDirty = false;

        spdlog::debug("PhysicsSystem: Updated properties for body {} (entity {})",
            bodyID.GetIndexAndSequenceNumber(), entity.get_uuid());
    }
}

void PhysicsSystem::UpdateBodyProperties(JPH::BodyID bodyID, const RigidBodyComponent& rb) {
    if (!m_bodyInterface || bodyID.IsInvalid()) return;

    // Check if body is currently active (needed for reactivation later)
    bool wasActive = m_bodyInterface->IsActive(bodyID);
    JPH::EMotionType currentMotionType = m_bodyInterface->GetMotionType(bodyID);
    JPH::EMotionType newMotionType = rb.ToJoltMotionType();

    // Deactivate body before making changes (required by Jolt for certain operations)
    if (wasActive) {
        m_bodyInterface->DeactivateBody(bodyID);
    }

    // If motion type is changing, we need to handle it specially
    if (currentMotionType != newMotionType) {
        // Remove body from physics world temporarily
        // Once removed, the body is not in the simulation and can be modified without locking
        m_bodyInterface->RemoveBody(bodyID);

        // Get the body pointer directly (no locking needed since it's removed from world)
        // We use TryGetBody to safely get the body pointer
        JPH::Body* body = m_physicsSystem->GetBodyLockInterface().TryGetBody(bodyID);
        if (body) {
            // Modify motion type directly (safe because body is removed from simulation)
            body->SetMotionType(newMotionType);

            // Re-add body to physics world (AddBody handles its own locking internally)
            m_bodyInterface->AddBody(bodyID, JPH::EActivation::DontActivate);

            spdlog::debug("PhysicsSystem: Changed motion type for body {} from {} to {}",
                bodyID.GetIndexAndSequenceNumber(),
                (int)currentMotionType, (int)newMotionType);
        } else {
            spdlog::error("PhysicsSystem: Failed to get body {} after removal for motion type change",
                bodyID.GetIndexAndSequenceNumber());
            // Try to re-add anyway
            m_bodyInterface->AddBody(bodyID, JPH::EActivation::DontActivate);
            if (wasActive && rb.isActive) {
                m_bodyInterface->ActivateBody(bodyID);
            }
            return;
        }
    }

    // Lock the body for property modifications
    JPH::BodyLockWrite lock(m_physicsSystem->GetBodyLockInterface(), bodyID);
    if (!lock.Succeeded()) {
        spdlog::warn("PhysicsSystem: Failed to lock body {} for property update",
            bodyID.GetIndexAndSequenceNumber());
        if (wasActive && rb.isActive) {
            m_bodyInterface->ActivateBody(bodyID);
        }
        return;
    }

    JPH::Body& body = lock.GetBody();

    // Update friction (can be updated on any body type)
    body.SetFriction(rb.friction);

    // Only update dynamic/kinematic properties if body is not static
    if (rb.motionType != RigidBodyComponent::MotionType::Static) {
        // Body has motion properties, safe to access
        JPH::MotionProperties* motionProps = body.GetMotionProperties();

        if (motionProps) {
            // Update mass (only for dynamic bodies)
            if (rb.motionType == RigidBodyComponent::MotionType::Dynamic) {
                JPH::MassProperties massProps = body.GetShape()->GetMassProperties();
                massProps.ScaleToMass(rb.mass);
                motionProps->SetMassProperties(JPH::EAllowedDOFs::All, massProps);
            }

            // Update linear damping
            motionProps->SetLinearDamping(rb.linearDamping);

            // Update angular damping
            motionProps->SetAngularDamping(rb.angularDrag);

            // Update gravity factor
            motionProps->SetGravityFactor(rb.useGravity ? rb.gravityFactor : 0.0f);

            // Update velocities (only meaningful for dynamic/kinematic bodies)
            // Note: Velocity constraints are applied in SyncTransformsFromPhysics, not here
            body.SetLinearVelocity(PhysicsUtils::ToJolt(rb.linearVelocity));
            body.SetAngularVelocity(PhysicsUtils::ToJolt(rb.angularVelocity));

            // Note: Position/rotation/velocity constraints are enforced in SyncTransformsFromPhysics
            // during the main physics update loop. This ensures constraints are applied every frame.
        }
    } else {
        // Static body - only friction can be updated
        spdlog::debug("PhysicsSystem: Skipping dynamic properties update for static body {}",
            bodyID.GetIndexAndSequenceNumber());
    }
    lock.ReleaseLock();
    // Reactivate body if it should be active
    if (rb.isActive) {
        m_bodyInterface->ActivateBody(bodyID);
    }
}

void PhysicsSystem::SyncDirtyColliders(ecs::world& world) {
    // Check BoxCollider (with or without RigidBodyComponent)
    {
        auto entities = world.filter_entities<BoxCollider>();
        for (auto const& entity : entities) {
            auto& collider = entity.get<BoxCollider>();

            if (collider.isDirty) {
                RecreateBodyWithNewShape(entity, world);
                collider.isDirty = false;
            }
        }
    }

    // Check SphereCollider (with or without RigidBodyComponent)
    {
        auto entities = world.filter_entities<SphereCollider>();
        for (auto const& entity : entities) {
            auto& collider = entity.get<SphereCollider>();

            if (collider.isDirty) {
                RecreateBodyWithNewShape(entity, world);
                collider.isDirty = false;
            }
        }
    }

    // Check CapsuleCollider (with or without RigidBodyComponent)
    {
        auto entities = world.filter_entities<CapsuleCollider>();
        for (auto const& entity : entities) {
            auto& collider = entity.get<CapsuleCollider>();

            if (collider.isDirty) {
                RecreateBodyWithNewShape(entity, world);
                collider.isDirty = false;
            }
        }
    }

    // Check MeshCollider (with or without RigidBodyComponent)
    {
        auto entities = world.filter_entities<MeshCollider>();
        for (auto const& entity : entities) {
            auto& collider = entity.get<MeshCollider>();

            if (collider.isDirty) {
                RecreateBodyWithNewShape(entity, world);
                collider.isDirty = false;
            }
        }
    }
}

void PhysicsSystem::RecreateBodyWithNewShape(ecs::entity entity, ecs::world& world) {
    spdlog::debug("PhysicsSystem: Recreating body with new shape for entity {}",
        entity.get_uuid());

    // Check if body exists
    auto it = m_entityToBodyID.find(entity);
    if (it == m_entityToBodyID.end()) {
        // No body exists yet, try to create one
        TryCreateBodyForEntity(entity);
        return;
    }

    JPH::BodyID oldBodyID = it->second;

    // Save current state before destroying
    JPH::Vec3 position{};
    JPH::Quat rotation{};
    JPH::Vec3 linearVel{};
    JPH::Vec3 angularVel{};
    bool wasActive = false;

    if (!oldBodyID.IsInvalid()) {
        position = m_bodyInterface->GetPosition(oldBodyID);
        rotation = m_bodyInterface->GetRotation(oldBodyID);
        linearVel = m_bodyInterface->GetLinearVelocity(oldBodyID);
        angularVel = m_bodyInterface->GetAngularVelocity(oldBodyID);
        wasActive = m_bodyInterface->IsActive(oldBodyID);

        // Destroy old body
        m_bodyInterface->RemoveBody(oldBodyID);
        m_bodyInterface->DestroyBody(oldBodyID);

        // Remove from maps
        m_entityToBodyID.erase(entity);
        m_bodyIDToEntity.erase(oldBodyID.GetIndexAndSequenceNumber());
    }

    // Create new body with updated shape
    TryCreateBodyForEntity(entity);

    // Restore state to new body
    auto newIt = m_entityToBodyID.find(entity);
    if (newIt != m_entityToBodyID.end() && !newIt->second.IsInvalid()) {
        JPH::BodyID newBodyID = newIt->second;

        // Restore position/rotation
        m_bodyInterface->SetPositionAndRotation(newBodyID, position, rotation, wasActive ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);

        // Restore velocities
        m_bodyInterface->SetLinearVelocity(newBodyID, linearVel);
        m_bodyInterface->SetAngularVelocity(newBodyID, angularVel);

        spdlog::debug("PhysicsSystem: Recreated body {} for {} for entity {}", oldBodyID.GetIndexAndSequenceNumber(), newBodyID.GetIndexAndSequenceNumber(), entity.get_uuid());
    }
}

void PhysicsSystem::FitEntityColliderToMesh(ecs::entity ent, glm::vec3 scl) {
    if (HasCollider(ent)) {
        ColliderFit fit = GetMeshFittings(ent);
        if (ent.all<BoxCollider>()) {
            auto& box = ent.get<BoxCollider>();
            box.size = fit.extents * scl;
            box.center = fit.offset * scl;
        }
        if (ent.all<SphereCollider>()) {
            auto& sph = ent.get<SphereCollider>();
            sph.radius = glm::compMax(fit.extents * scl) / 2.f; //use ritters or something for a btr bv
            sph.center = fit.offset * scl;
        }
        if (ent.all<CapsuleCollider>()) {
            auto& cap = ent.get<CapsuleCollider>();
            cap.mRadius = std::max(fit.extents.x*scl.x, fit.extents.z*scl.z)/2.f; //use ritters or something for a btr bv
            cap.mHeight = fit.extents.y * scl.y;
            cap.center = fit.offset * scl;
        }
        if (ent.all<MeshCollider>()) {
            //return &ent.get<MeshCollider>();
        }
        if (auto bodyId = GetBodyID(ent); !bodyId.IsInvalid()) {
            m_bodyInterface->SetShape(bodyId, CreateShapeFromCollider(ent), true, JPH::EActivation::Activate);
        }
    }
}

bool PhysicsSystem::HasCollider(ecs::entity ent) {
    return ent.any<BoxCollider, SphereCollider, CapsuleCollider, MeshCollider>();
}

CollisionBase* PhysicsSystem::GetColliderBasePtr(ecs::entity ent) {
    if (ent.all<BoxCollider>()) {
        return &ent.get<BoxCollider>();
    }
    if (ent.all<SphereCollider>()) {
        return &ent.get<SphereCollider>();
    }
    if (ent.all<CapsuleCollider>()) {
        return &ent.get<CapsuleCollider>();
    }
    if (ent.all<MeshCollider>()) {
        return &ent.get<MeshCollider>();
    }
    return nullptr;
}

// =============================================================================
// Debug Rendering Configuration
// =============================================================================

// ============================================================================
// RAYCASTING IMPLEMENTATION
// ============================================================================
//
// Usage Example for Player Interaction:
//
// // Get player camera position and look direction
// glm::vec3 cameraPos = playerCamera.GetPosition();
// glm::vec3 lookDir = playerCamera.GetForward();
//
// // Raycast from camera forward
// auto hit = PhysicsSystem::Instance().Raycast(cameraPos, lookDir, 5.0f);
//
// if (hit.hasHit) {
//     // Player is looking at an object
//     std::cout << "Looking at entity: " << hit.entity.name() << std::endl;
//     std::cout << "Distance: " << hit.distance << "m" << std::endl;
//
//     // Show UI prompt
//     if (hit.distance < 2.0f) {
//         ShowInteractionPrompt(hit.entity);
//     }
// }
//
// // Alternative: Ignore the player's own body
// std::vector<ecs::entity> ignore = { playerEntity };
// auto hit = PhysicsSystem::Instance().RaycastIgnoring(cameraPos, lookDir, ignore, 5.0f);
//
// ============================================================================

PhysicsSystem::RaycastHit PhysicsSystem::Raycast(
    const glm::vec3& origin,
    const glm::vec3& direction,
    float maxDistance,
    bool ignoreTriggers)
{
    RaycastHit result;

    if (!m_physicsSystem) {
        spdlog::warn("PhysicsSystem::Raycast - Physics system not initialized");
        return result;
    }

    // Normalize direction
    glm::vec3 normalizedDir = glm::normalize(direction);

    // Setup Jolt raycast
    JPH::RRayCast ray;
    ray.mOrigin = PhysicsUtils::ToJolt(origin);
    ray.mDirection = PhysicsUtils::ToJolt(normalizedDir * maxDistance);

    // Settings for raycast
    JPH::RayCastSettings settings;

    // Collector for closest hit
    class ClosestHitCollector : public JPH::CastRayCollector
    {
    public:
        bool ignoreTriggers;
        PhysicsSystem* physicsSystem;
        JPH::RayCastResult mHit;  // Store the closest hit
        bool mHadHit = false;     // Track if we had any hit

        ClosestHitCollector(bool ignoreTrig, PhysicsSystem* sys)
            : ignoreTriggers(ignoreTrig), physicsSystem(sys) {}

        void AddHit(const JPH::RayCastResult& result) override
        {
            // Check if we should ignore triggers
            if (ignoreTriggers) {
                JPH::BodyID bodyID = result.mBodyID;
                ecs::entity entity = physicsSystem->GetEntityFromBodyID(bodyID);
                if (entity && physicsSystem->IsEntityTrigger(entity)) {
                    return; // Skip triggers
                }
            }

            // Store this hit if it's the first or closer than previous
            if (!mHadHit || result.mFraction < mHit.mFraction) {
                mHit = result;
                mHadHit = true;
                UpdateEarlyOutFraction(result.mFraction);
            }
        }
    };

    ClosestHitCollector collector(ignoreTriggers, this);

    // Perform raycast
    m_physicsSystem->GetNarrowPhaseQuery().CastRay(ray, settings, collector);

    // Check if we hit something
    if (collector.mHadHit)
    {
        result.hasHit = true;

        // Get body ID from the hit
        JPH::BodyID hitBodyID = collector.mHit.mBodyID;
        result.bodyID = hitBodyID;

        // Get entity from body ID
        result.entity = GetEntityFromBodyID(hitBodyID);

        // Calculate hit point
        result.distance = collector.mHit.mFraction * maxDistance;
        result.hitPoint = origin + normalizedDir * result.distance;

        // Get surface normal
        JPH::BodyLockRead lock(m_physicsSystem->GetBodyLockInterface(), hitBodyID);
        if (lock.Succeeded())
        {
            const JPH::Body& body = lock.GetBody();

            // Get the surface normal at the hit point
            JPH::Vec3 joltHitPoint = PhysicsUtils::ToJolt(result.hitPoint);
            JPH::Vec3 joltNormal = body.GetWorldSpaceSurfaceNormal(collector.mHit.mSubShapeID2, joltHitPoint);
            result.hitNormal = PhysicsUtils::ToGLM(joltNormal);

            // Check if it's a trigger
            result.isTrigger = body.IsSensor();
        }

        spdlog::debug("PhysicsSystem::Raycast - Hit entity {} at distance {}",
            result.entity.get_uuid(), result.distance);
    }

    return result;
}

PhysicsSystem::RaycastHit PhysicsSystem::RaycastIgnoring(
    const glm::vec3& origin,
    const glm::vec3& direction,
    const std::vector<ecs::entity>& ignoreEntities,
    float maxDistance,
    bool ignoreTriggers)
{
    RaycastHit result;

    if (!m_physicsSystem) {
        spdlog::warn("PhysicsSystem::RaycastIgnoring - Physics system not initialized");
        return result;
    }

    // Normalize direction
    glm::vec3 normalizedDir = glm::normalize(direction);

    // Setup Jolt raycast
    JPH::RRayCast ray;
    ray.mOrigin = PhysicsUtils::ToJolt(origin);
    ray.mDirection = PhysicsUtils::ToJolt(normalizedDir * maxDistance);

    // Settings for raycast
    JPH::RayCastSettings settings;

    // Collector for closest hit with ignore list
    class IgnoreListCollector : public JPH::CastRayCollector
    {
    public:
        bool ignoreTriggers;
        PhysicsSystem* physicsSystem;
        const std::vector<ecs::entity>& ignoreList;
        JPH::RayCastResult mHit;  // Store the closest hit
        bool mHadHit = false;     // Track if we had any hit

        IgnoreListCollector(bool ignoreTrig, PhysicsSystem* sys, const std::vector<ecs::entity>& ignore)
            : ignoreTriggers(ignoreTrig), physicsSystem(sys), ignoreList(ignore) {}

        void AddHit(const JPH::RayCastResult& result) override
        {
            JPH::BodyID bodyID = result.mBodyID;
            ecs::entity entity = physicsSystem->GetEntityFromBodyID(bodyID);

            // Skip entities in ignore list
            if (entity) {
                for (const auto& ignoreEntity : ignoreList) {
                    if (entity == ignoreEntity) {
                        return; // Skip this entity
                    }
                }

                // Check if we should ignore triggers
                if (ignoreTriggers && physicsSystem->IsEntityTrigger(entity)) {
                    return; // Skip triggers
                }
            }

            // Store this hit if it's the first or closer than previous
            if (!mHadHit || result.mFraction < mHit.mFraction) {
                mHit = result;
                mHadHit = true;
                UpdateEarlyOutFraction(result.mFraction);
            }
        }
    };

    IgnoreListCollector collector(ignoreTriggers, this, ignoreEntities);

    // Perform raycast
    m_physicsSystem->GetNarrowPhaseQuery().CastRay(ray, settings, collector);

    // Check if we hit something
    if (collector.mHadHit)
    {
        result.hasHit = true;

        JPH::BodyID hitBodyID = collector.mHit.mBodyID;
        result.bodyID = hitBodyID;
        result.entity = GetEntityFromBodyID(hitBodyID);

        result.distance = collector.mHit.mFraction * maxDistance;
        result.hitPoint = origin + normalizedDir * result.distance;

        JPH::BodyLockRead lock(m_physicsSystem->GetBodyLockInterface(), hitBodyID);
        if (lock.Succeeded())
        {
            const JPH::Body& body = lock.GetBody();
            JPH::Vec3 joltHitPoint = PhysicsUtils::ToJolt(result.hitPoint);
            JPH::Vec3 joltNormal = body.GetWorldSpaceSurfaceNormal(collector.mHit.mSubShapeID2, joltHitPoint);
            result.hitNormal = PhysicsUtils::ToGLM(joltNormal);
            result.isTrigger = body.IsSensor();
        }

        spdlog::debug("PhysicsSystem::RaycastIgnoring - Hit entity {} at distance {}",
            result.entity.get_uuid(), result.distance);
    }

    return result;
}

std::vector<PhysicsSystem::RaycastHit> PhysicsSystem::RaycastAll(
    const glm::vec3& origin,
    const glm::vec3& direction,
    float maxDistance,
    bool ignoreTriggers)
{
    std::vector<RaycastHit> results;

    if (!m_physicsSystem) {
        spdlog::warn("PhysicsSystem::RaycastAll - Physics system not initialized");
        return results;
    }

    // Normalize direction
    glm::vec3 normalizedDir = glm::normalize(direction);

    // Setup Jolt raycast
    JPH::RRayCast ray;
    ray.mOrigin = PhysicsUtils::ToJolt(origin);
    ray.mDirection = PhysicsUtils::ToJolt(normalizedDir * maxDistance);

    // Settings for raycast
    JPH::RayCastSettings settings;

    // Collector for all hits
    class AllHitsCollector : public JPH::CastRayCollector
    {
    public:
        bool ignoreTriggers;
        PhysicsSystem* physicsSystem;
        std::vector<JPH::RayCastResult> hits;

        AllHitsCollector(bool ignoreTrig, PhysicsSystem* sys)
            : ignoreTriggers(ignoreTrig), physicsSystem(sys) {}

        void AddHit(const JPH::RayCastResult& result) override
        {
            // Check if we should ignore triggers
            if (ignoreTriggers) {
                JPH::BodyID bodyID = result.mBodyID;
                ecs::entity entity = physicsSystem->GetEntityFromBodyID(bodyID);
                if (entity && physicsSystem->IsEntityTrigger(entity)) {
                    return; // Skip triggers
                }
            }

            // Store all hits
            hits.push_back(result);
        }
    };

    AllHitsCollector collector(ignoreTriggers, this);

    // Perform raycast
    m_physicsSystem->GetNarrowPhaseQuery().CastRay(ray, settings, collector);

    // Convert all Jolt hits to our RaycastHit structure
    for (const auto& joltHit : collector.hits)
    {
        RaycastHit hit;
        hit.hasHit = true;

        JPH::BodyID hitBodyID = joltHit.mBodyID;
        hit.bodyID = hitBodyID;
        hit.entity = GetEntityFromBodyID(hitBodyID);

        hit.distance = joltHit.mFraction * maxDistance;
        hit.hitPoint = origin + normalizedDir * hit.distance;

        JPH::BodyLockRead lock(m_physicsSystem->GetBodyLockInterface(), hitBodyID);
        if (lock.Succeeded())
        {
            const JPH::Body& body = lock.GetBody();
            JPH::Vec3 joltHitPoint = PhysicsUtils::ToJolt(hit.hitPoint);
            JPH::Vec3 joltNormal = body.GetWorldSpaceSurfaceNormal(joltHit.mSubShapeID2, joltHitPoint);
            hit.hitNormal = PhysicsUtils::ToGLM(joltNormal);
            hit.isTrigger = body.IsSensor();
        }

        results.push_back(hit);
    }

    // Sort by distance (closest first)
    std::sort(results.begin(), results.end(),
        [](const RaycastHit& a, const RaycastHit& b) { return a.distance < b.distance; });

    spdlog::debug("PhysicsSystem::RaycastAll - Found {} hits", results.size());

    return results;
}
