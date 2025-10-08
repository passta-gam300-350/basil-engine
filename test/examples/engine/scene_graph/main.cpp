#include <iostream>
#include <iomanip>
#include "Scene/SceneGraph.hpp"
#include "Component/RelationshipComponent.hpp"
#include "components/transform.h"
#include "System/TransformHierarchySystem.hpp"
#include "Ecs/ecs.h"
#include <glm/gtc/matrix_transform.hpp>

void PrintTransform(const std::string& name, ecs::entity entity) {
    auto& transform = entity.get<TransformComponent>();
    glm::vec3 worldPos = glm::vec3(transform.worldMatrix[3]);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << name << ":\n";
    std::cout << "  Local Pos: (" << transform.localPosition.x << ", "
              << transform.localPosition.y << ", " << transform.localPosition.z << ")\n";
    std::cout << "  World Pos: (" << worldPos.x << ", " << worldPos.y << ", " << worldPos.z << ")\n";
}

int main() {
    std::cout << "=== Scene Graph System Example ===\n\n";

    // Create world
    ecs::world world = ecs::world::new_world_instance();

    // Create entities
    std::cout << "Creating entities...\n";
    ecs::entity sun = world.add_entity();
    sun.add<TransformComponent>();
    sun.add<RelationshipComponent>();

    ecs::entity earth = world.add_entity();
    earth.add<TransformComponent>();
    earth.add<RelationshipComponent>();

    ecs::entity moon = world.add_entity();
    moon.add<TransformComponent>();
    moon.add<RelationshipComponent>();

    // Set up hierarchy: Sun -> Earth -> Moon
    std::cout << "\nSetting up hierarchy: Sun -> Earth -> Moon\n";
    SceneGraph::SetParent(earth, sun);
    SceneGraph::SetParent(moon, earth);

    // Configure transforms
    auto& sunTransform = sun.get<TransformComponent>();
    sunTransform.localPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    sunTransform.localScale = glm::vec3(3.0f);

    auto& earthTransform = earth.get<TransformComponent>();
    earthTransform.localPosition = glm::vec3(10.0f, 0.0f, 0.0f); // 10 units from sun
    earthTransform.localScale = glm::vec3(1.0f);

    auto& moonTransform = moon.get<TransformComponent>();
    moonTransform.localPosition = glm::vec3(2.0f, 0.0f, 0.0f); // 2 units from earth
    moonTransform.localScale = glm::vec3(0.3f);

    // Create and run transform system
    TransformHierarchySystem transformSystem;
    transformSystem.Init();

    std::cout << "\n--- Initial State ---\n";
    transformSystem.Update(world, 0.016f);
    PrintTransform("Sun", sun);
    PrintTransform("Earth", earth);
    PrintTransform("Moon", moon);

    // Move the sun
    std::cout << "\n--- After moving Sun to (5, 0, 0) ---\n";
    sunTransform.localPosition = glm::vec3(5.0f, 0.0f, 0.0f);
    SceneGraph::MarkTransformDirty(sun);
    transformSystem.Update(world, 0.016f);
    PrintTransform("Sun", sun);
    PrintTransform("Earth", earth);
    PrintTransform("Moon", moon);

    // Scale the sun
    std::cout << "\n--- After scaling Sun by 2x ---\n";
    sunTransform.localScale = glm::vec3(6.0f);
    SceneGraph::MarkTransformDirty(sun);
    transformSystem.Update(world, 0.016f);

    // Extract scale from matrices
    auto extractScale = [](const glm::mat4& mat) {
        return glm::length(glm::vec3(mat[0]));
    };

    std::cout << "Sun scale: " << extractScale(sunTransform.worldMatrix) << "\n";
    std::cout << "Earth scale: " << extractScale(earthTransform.worldMatrix) << "\n";
    std::cout << "Moon scale: " << extractScale(moonTransform.worldMatrix) << "\n";

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
    carTransform.localPosition = glm::vec3(0.0f, 1.0f, 0.0f);

    auto& wheel1Transform = wheel1.get<TransformComponent>();
    wheel1Transform.localPosition = glm::vec3(-1.5f, -0.5f, 1.0f);

    auto& wheel2Transform = wheel2.get<TransformComponent>();
    wheel2Transform.localPosition = glm::vec3(1.5f, -0.5f, 1.0f);

    transformSystem.Update(world, 0.016f);
    PrintTransform("Car", car);
    PrintTransform("Wheel 1", wheel1);
    PrintTransform("Wheel 2", wheel2);

    std::cout << "\n--- Moving Car ---\n";
    carTransform.localPosition = glm::vec3(10.0f, 1.0f, 5.0f);
    SceneGraph::MarkTransformDirty(car);
    transformSystem.Update(world, 0.016f);
    PrintTransform("Car", car);
    PrintTransform("Wheel 1 (moves with car)", wheel1);
    PrintTransform("Wheel 2 (moves with car)", wheel2);

    std::cout << "\n=== Scene Graph Example Complete ===\n";

    // Cleanup
    transformSystem.Exit();
    world.destroy_world();

    return 0;
}
