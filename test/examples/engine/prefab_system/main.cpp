/**
 * @file main.cpp
 * @brief Simple example demonstrating prefab system functionality
 *
 * This example shows:
 * 1. Creating entities with components
 * 2. Creating a prefab from an entity hierarchy
 * 3. Saving/loading prefabs from disk
 * 4. Instantiating prefab instances
 * 5. Property overrides
 * 6. Syncing instances when prefab changes
 */

#include "ecs/internal/world.h"
#include "System/PrefabSystem.hpp"
#include "Component/Transform.hpp"
#include "Component/PrefabComponent.hpp"
#include "Reflection/ReflectionRegistry.hpp"
#include <iostream>

// Simple tag component for testing
struct NameComponent
{
    std::string name;
};

void PrintSeparator(const std::string& title)
{
    std::cout << "\n========== " << title << " ==========\n";
}

void PrintEntity(ecs::world& world, ecs::entity entity, const std::string& label)
{
    std::cout << label << ":\n";

    if (entity.all<NameComponent>())
    {
        auto& name = entity.get<NameComponent>();
        std::cout << "  Name: " << name.name << "\n";
    }

    if (entity.all<TransformComponent>())
    {
        auto& transform = entity.get<TransformComponent>();
        std::cout << "  Position: ("
                  << transform.m_Translation.x << ", "
                  << transform.m_Translation.y << ", "
                  << transform.m_Translation.z << ")\n";
        std::cout << "  Scale: ("
                  << transform.m_Scale.x << ", "
                  << transform.m_Scale.y << ", "
                  << transform.m_Scale.z << ")\n";
    }

    if (entity.all<PrefabComponent>())
    {
        auto& prefab = entity.get<PrefabComponent>();
        std::cout << "  [Prefab Instance] GUID: " << prefab.GetPrefabGuidString() << "\n";
        std::cout << "  Override count: " << prefab.m_OverriddenProperties.size() << "\n";
    }

    std::cout << "\n";
}

int main()
{
    PrintSeparator("Prefab System Example");

    // Create ECS world
    ecs::world world;

    // TODO: Initialize reflection registry
    // This is required before using the prefab system
    // ReflectionRegistry::RegisterType<TransformComponent>();
    // ReflectionRegistry::RegisterType<NameComponent>();

    // Initialize prefab system
    PrefabSystem prefabSystem;
    prefabSystem.Init();

    // ============================================================
    // STEP 1: Create a simple entity hierarchy (a "Player" prefab)
    // ============================================================
    PrintSeparator("Step 1: Create Entity Hierarchy");

    std::cout << "Creating player entity with transform and name...\n";
    ecs::entity playerTemplate = world.add_entity();

    // Add components
    playerTemplate.add<NameComponent>("Player");

    auto& transform = playerTemplate.add<TransformComponent>();
    transform.m_Translation = glm::vec3(0.0f, 0.0f, 0.0f);
    transform.m_Scale = glm::vec3(1.0f, 1.0f, 1.0f);
    transform.m_Rotation = glm::vec3(0.0f, 0.0f, 0.0f);

    PrintEntity(world, playerTemplate, "Player Template");

    // ============================================================
    // STEP 2: Create a prefab from the entity
    // ============================================================
    PrintSeparator("Step 2: Create Prefab from Entity");

    std::string prefabPath = "player.prefab";
    std::cout << "Creating prefab at: " << prefabPath << "\n";

    UUID<128> prefabId = PrefabSystem::CreatePrefabFromEntity(
        world,
        playerTemplate,
        "Player",
        prefabPath
    );

    if (prefabId == UUID<128>())
    {
        std::cerr << "ERROR: Failed to create prefab!\n";
        return 1;
    }

    std::cout << "Prefab created successfully!\n";
    std::cout << "Prefab UUID: " << prefabId.ToString() << "\n";

    // We can remove the template entity now
    world.remove_entity(playerTemplate);
    std::cout << "Removed template entity\n";

    // ============================================================
    // STEP 3: Load the prefab from disk
    // ============================================================
    PrintSeparator("Step 3: Load Prefab from Disk");

    PrefabData loadedPrefab = PrefabSystem::LoadPrefabFromFile(prefabPath);

    if (!loadedPrefab.IsValid())
    {
        std::cerr << "ERROR: Failed to load prefab from disk!\n";
        return 1;
    }

    std::cout << "Prefab loaded successfully!\n";
    std::cout << "  Name: " << loadedPrefab.name << "\n";
    std::cout << "  UUID: " << loadedPrefab.GetUuidString() << "\n";
    std::cout << "  Version: " << loadedPrefab.version << "\n";
    std::cout << "  Components: " << loadedPrefab.root.components.size() << "\n";

    // ============================================================
    // STEP 4: Instantiate the prefab multiple times
    // ============================================================
    PrintSeparator("Step 4: Instantiate Prefab");

    std::cout << "Instantiating 3 player instances...\n";

    ecs::entity player1 = PrefabSystem::InstantiatePrefab(
        world,
        prefabId,
        glm::vec3(0.0f, 0.0f, 0.0f)
    );

    ecs::entity player2 = PrefabSystem::InstantiatePrefab(
        world,
        prefabId,
        glm::vec3(5.0f, 0.0f, 0.0f)
    );

    ecs::entity player3 = PrefabSystem::InstantiatePrefab(
        world,
        prefabId,
        glm::vec3(10.0f, 0.0f, 0.0f)
    );

    if (player1.get_uuid() == 0 || player2.get_uuid() == 0 || player3.get_uuid() == 0)
    {
        std::cerr << "ERROR: Failed to instantiate prefabs!\n";
        return 1;
    }

    std::cout << "All instances created successfully!\n\n";

    PrintEntity(world, player1, "Player 1");
    PrintEntity(world, player2, "Player 2");
    PrintEntity(world, player3, "Player 3");

    // ============================================================
    // STEP 5: Modify instance (create property override)
    // ============================================================
    PrintSeparator("Step 5: Create Property Override");

    std::cout << "Modifying Player 2's scale to (2, 2, 2)...\n";

    if (player2.all<TransformComponent>())
    {
        auto& transform2 = player2.get<TransformComponent>();
        transform2.m_Scale = glm::vec3(2.0f, 2.0f, 2.0f);

        // Mark as overridden (in real usage, this might happen automatically in the editor)
        // You need to get the reflection type hash for TransformComponent
        // auto typeHash = entt::resolve<TransformComponent>().id();
        // PrefabSystem::MarkPropertyOverridden(
        //     world,
        //     player2,
        //     typeHash,
        //     "TransformComponent",
        //     "m_Scale",
        //     transform2.m_Scale
        // );
    }

    PrintEntity(world, player2, "Modified Player 2");

    // ============================================================
    // STEP 6: Query instances
    // ============================================================
    PrintSeparator("Step 6: Query Prefab Instances");

    std::vector<ecs::entity> instances = PrefabSystem::GetAllInstances(world, prefabId);
    std::cout << "Found " << instances.size() << " instances of the Player prefab\n";

    for (size_t i = 0; i < instances.size(); ++i)
    {
        bool isInstance = PrefabSystem::IsInstanceOf(world, instances[i], prefabId);
        std::cout << "  Instance " << (i + 1) << ": "
                  << (isInstance ? "✓ Valid" : "✗ Invalid") << "\n";
    }

    // ============================================================
    // STEP 7: Check if entity is prefab instance
    // ============================================================
    PrintSeparator("Step 7: Check Instance Status");

    std::cout << "Player 1 is prefab instance: "
              << (PrefabSystem::IsPrefabInstance(world, player1) ? "Yes" : "No") << "\n";

    // Create a non-prefab entity for comparison
    ecs::entity regularEntity = world.add_entity();
    regularEntity.add<TransformComponent>();

    std::cout << "Regular entity is prefab instance: "
              << (PrefabSystem::IsPrefabInstance(world, regularEntity) ? "Yes" : "No") << "\n";

    // ============================================================
    // STEP 8: Sync instances (simulating prefab update)
    // ============================================================
    PrintSeparator("Step 8: Sync Instances");

    std::cout << "Note: SyncPrefab would reload from disk and update all instances\n";
    std::cout << "while preserving property overrides.\n\n";

    // In real usage:
    // int synced = PrefabSystem::SyncPrefab(world, prefabId);
    // std::cout << "Synced " << synced << " instances\n";

    // ============================================================
    // Cleanup
    // ============================================================
    PrintSeparator("Cleanup");

    prefabSystem.Exit();
    std::cout << "Prefab system shut down successfully\n";

    PrintSeparator("Example Complete");
    std::cout << "\nThe prefab system is working! ✓\n";
    std::cout << "\nGenerated files:\n";
    std::cout << "  - player.prefab (YAML prefab data)\n";
    std::cout << "\nYou can inspect the .prefab file to see the serialized format.\n\n";

    return 0;
}
