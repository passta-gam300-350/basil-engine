#include <iostream>
#include <iomanip>
#include "Scene/SceneGraph.hpp"
#include "Component/RelationshipComponent.hpp"
#include "Component/Transform.hpp"
#include "System/HierarchySystem.hpp"
#include "Ecs/ecs.h"
#include <glm/gtc/matrix_transform.hpp>

void PrintTransform(const std::string& name, ecs::entity entity) {
    auto& transform = entity.get<TransformComponent>();

    glm::vec3 worldPos(0.0f);
    if (entity.all<TransformMtxComponent>()) {
        auto& mtx = entity.get<TransformMtxComponent>();
        worldPos = glm::vec3(mtx.m_Mtx[3]);
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << name << ":\n";
    std::cout << "  Local Pos: (" << transform.m_Translation.x << ", "
              << transform.m_Translation.y << ", " << transform.m_Translation.z << ")\n";
    std::cout << "  World Pos: (" << worldPos.x << ", " << worldPos.y << ", " << worldPos.z << ")\n";
}

int main() {
    std::cout << "=== Scene Graph System Example ===\n\n";

    // Create world
    ecs::world world = ecs::world::new_world_instance();

    // Create entities
    std::cout << "Creating entities...\n";
    ecs::entity sun = world.add_entity();
    std::cout << "  Created sun, adding components...\n";
    sun.add<TransformComponent>();
    sun.add<RelationshipComponent>();
    std::cout << "    Added RelationshipComponent\n";

    ecs::entity earth = world.add_entity();
    std::cout << "  Created earth, adding components...\n";
    earth.add<TransformComponent>();
    std::cout << "    Added TransformComponent\n";
    earth.add<RelationshipComponent>();
    std::cout << "    Added RelationshipComponent\n";

    ecs::entity moon = world.add_entity();
    std::cout << "  Created moon, adding components...\n";
    moon.add<TransformComponent>();
    std::cout << "    Added TransformComponent\n";
    moon.add<RelationshipComponent>();
    std::cout << "    Added RelationshipComponent\n";
    std::cout << "All entities created successfully.\n";

    // Set up hierarchy: Sun -> Earth -> Moon
    std::cout << "\nSetting up hierarchy: Sun -> Earth -> Moon\n";
    SceneGraph::SetParent(earth, sun);
    SceneGraph::SetParent(moon, earth);

    // Configure transforms
    auto& sunTransform = sun.get<TransformComponent>();
    sunTransform.m_Translation = glm::vec3(0.0f, 0.0f, 0.0f);
    sunTransform.m_Scale = glm::vec3(3.0f);

    auto& earthTransform = earth.get<TransformComponent>();
    earthTransform.m_Translation = glm::vec3(10.0f, 0.0f, 0.0f); // 10 units from sun
    earthTransform.m_Scale = glm::vec3(1.0f);

    auto& moonTransform = moon.get<TransformComponent>();
    moonTransform.m_Translation = glm::vec3(2.0f, 0.0f, 0.0f); // 2 units from earth
    moonTransform.m_Scale = glm::vec3(0.3f);

    // Create and run hierarchy system
    HierarchySystem transformSystem;
    transformSystem.Init();

    std::cout << "\n--- Initial State ---\n";
    transformSystem.Update(world, 0.016f);
    PrintTransform("Sun", sun);
    PrintTransform("Earth", earth);
    PrintTransform("Moon", moon);

    // Move the sun
    std::cout << "\n--- After moving Sun to (5, 0, 0) ---\n";
    sunTransform.m_Translation = glm::vec3(5.0f, 0.0f, 0.0f);
    SceneGraph::MarkTransformDirty(sun);
    transformSystem.Update(world, 0.016f);
    PrintTransform("Sun", sun);
    PrintTransform("Earth", earth);
    PrintTransform("Moon", moon);

    // Scale the sun
    std::cout << "\n--- After scaling Sun by 2x ---\n";
    sunTransform.m_Scale = glm::vec3(6.0f);
    SceneGraph::MarkTransformDirty(sun);
    transformSystem.Update(world, 0.016f);

    // Extract scale from matrices
    auto extractScale = [](ecs::entity ent) {
        if (ent.all<TransformMtxComponent>()) {
            auto& mtx = ent.get<TransformMtxComponent>();
            return glm::length(glm::vec3(mtx.m_Mtx[0]));
        }
        return 1.0f;
    };

    std::cout << "Sun scale: " << extractScale(sun) << "\n";
    std::cout << "Earth scale: " << extractScale(earth) << "\n";
    std::cout << "Moon scale: " << extractScale(moon) << "\n";

    // Print positions after scaling
    std::cout << "\nPositions after scaling:\n";
    PrintTransform("Earth", earth);
    PrintTransform("Moon", moon);

    // Test reparenting
    std::cout << "\n--- Reparenting Moon directly under Sun ---\n";
    SceneGraph::SetParent(moon, sun, true); // Keep world transform
    transformSystem.Update(world, 0.016f);
    PrintTransform("Moon (now child of Sun)", moon);

    // Test hierarchy queries
    std::cout << "\n--- Hierarchy Information ---\n";
    std::cout << "Sun has parent: " << (SceneGraph::HasParent(sun) ? "Yes" : "No") << "\n";
    std::cout << "Earth has parent: " << (SceneGraph::HasParent(earth) ? "Yes" : "No") << "\n";
    std::cout << "Sun's children: " << SceneGraph::GetChildren(sun).size() << "\n";
    std::cout << "Earth's children: " << SceneGraph::GetChildren(earth).size() << "\n";

    // Test creating a complex hierarchy
    std::cout << "\n--- Creating a Car with Wheels ---\n";
    ecs::entity car = world.add_entity();
    car.add<TransformComponent>();
    car.add<RelationshipComponent>();

    ecs::entity wheel1 = world.add_entity();
    wheel1.add<TransformComponent>();
    wheel1.add<RelationshipComponent>();

    ecs::entity wheel2 = world.add_entity();
    wheel2.add<TransformComponent>();
    wheel2.add<RelationshipComponent>();

    SceneGraph::AddChild(car, wheel1);
    SceneGraph::AddChild(car, wheel2);

    auto& carTransform = car.get<TransformComponent>();
    carTransform.m_Translation = glm::vec3(0.0f, 1.0f, 0.0f);

    auto& wheel1Transform = wheel1.get<TransformComponent>();
    wheel1Transform.m_Translation = glm::vec3(-1.5f, -0.5f, 1.0f);

    auto& wheel2Transform = wheel2.get<TransformComponent>();
    wheel2Transform.m_Translation = glm::vec3(1.5f, -0.5f, 1.0f);

    transformSystem.Update(world, 0.016f);
    PrintTransform("Car", car);
    PrintTransform("Wheel 1", wheel1);
    PrintTransform("Wheel 2", wheel2);

    std::cout << "\n--- Moving Car ---\n";
    carTransform.m_Translation = glm::vec3(10.0f, 1.0f, 5.0f);
    SceneGraph::MarkTransformDirty(car);
    transformSystem.Update(world, 0.016f);
    PrintTransform("Car", car);
    PrintTransform("Wheel 1 (moves with car)", wheel1);
    PrintTransform("Wheel 2 (moves with car)", wheel2);

    // Test deep hierarchy: Body -> Arm -> Forearm -> Hand -> Finger
    std::cout << "\n--- Testing Deep Hierarchy (5 levels) ---\n";
    ecs::entity body = world.add_entity();
    body.add<TransformComponent>();
    body.add<RelationshipComponent>();

    ecs::entity arm = world.add_entity();
    arm.add<TransformComponent>();
    arm.add<RelationshipComponent>();

    ecs::entity forearm = world.add_entity();
    forearm.add<TransformComponent>();
    forearm.add<RelationshipComponent>();

    ecs::entity hand = world.add_entity();
    hand.add<TransformComponent>();
    hand.add<RelationshipComponent>();

    ecs::entity finger = world.add_entity();
    finger.add<TransformComponent>();
    finger.add<RelationshipComponent>();

    // Build hierarchy
    SceneGraph::SetParent(arm, body);
    SceneGraph::SetParent(forearm, arm);
    SceneGraph::SetParent(hand, forearm);
    SceneGraph::SetParent(finger, hand);

    // Set transforms
    auto& bodyTransform = body.get<TransformComponent>();
    bodyTransform.m_Translation = glm::vec3(0.0f, 0.0f, 0.0f);

    auto& armTransform = arm.get<TransformComponent>();
    armTransform.m_Translation = glm::vec3(1.0f, 0.0f, 0.0f);

    auto& forearmTransform = forearm.get<TransformComponent>();
    forearmTransform.m_Translation = glm::vec3(1.0f, 0.0f, 0.0f);

    auto& handTransform = hand.get<TransformComponent>();
    handTransform.m_Translation = glm::vec3(1.0f, 0.0f, 0.0f);

    auto& fingerTransform = finger.get<TransformComponent>();
    fingerTransform.m_Translation = glm::vec3(1.0f, 0.0f, 0.0f);

    transformSystem.Update(world, 0.016f);

    std::cout << "5-level hierarchy positions:\n";
    PrintTransform("Body (root)", body);
    PrintTransform("Arm (level 2)", arm);
    PrintTransform("Forearm (level 3)", forearm);
    PrintTransform("Hand (level 4)", hand);
    PrintTransform("Finger (level 5)", finger);

    std::cout << "\nMoving body to (10, 5, 0) - all children should follow:\n";
    bodyTransform.m_Translation = glm::vec3(10.0f, 5.0f, 0.0f);
    SceneGraph::MarkTransformDirty(body);
    transformSystem.Update(world, 0.016f);

    PrintTransform("Body", body);
    PrintTransform("Finger (deepest child)", finger);

    std::cout << "\n=== Scene Graph Example Complete ===\n";

    // Cleanup
    transformSystem.Exit();
    world.destroy_world();

    return 0;
}
