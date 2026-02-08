/******************************************************************************/
/*!
\file   ecs_test.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  ECS unit tests

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include <gtest/gtest.h>
#include "Ecs/ecs.h"

using namespace ecs;

// Sample components for testing
struct Position { float x, y; };
struct Velocity { float dx, dy; };

TEST(WorldLifecycle, CreateAndDestroyWorld) {
    world w = world::new_world_instance();
    EXPECT_TRUE(w);
    w.destroy_world();
    EXPECT_FALSE(static_cast<bool>(w));
}

TEST(EntityManagement, AddAndRemoveEntity) {
    world w = world::new_world_instance();
    entity e = w.add_entity();
    EXPECT_TRUE(e);
    EXPECT_EQ(e.get_world_handle(), static_cast<std::uint32_t>(w));
    w.remove_entity(e);
    // Entity still exists but is invalidated
    EXPECT_FALSE(static_cast<bool>(e));
}

TEST(ComponentHandling, AddGetRemoveComponent) {
    world w = world::new_world_instance();
    entity e = w.add_entity();

    auto& pos = e.add<Position>(1.0f, 2.0f);
    EXPECT_EQ(pos.x, 1.0f);
    EXPECT_EQ(pos.y, 2.0f);

    auto& fetched = e.get<Position>();
    EXPECT_EQ(fetched.x, 1.0f);

    EXPECT_TRUE(e.all<Position>());
    EXPECT_FALSE(e.any<Velocity>());

    e.remove<Position>();
    EXPECT_FALSE(e.any<Position>());
}

/*
TEST(Hierarchy, ParentChildRelationship) {
    world w = world::new_world_instance();
    entity parent = w.add_entity();
    entity child = w.add_entity();

    child.set_parent(parent);
    EXPECT_EQ(child.parent(), parent);

    entity duplicated = parent.duplicate();
    EXPECT_TRUE(duplicated);
    EXPECT_EQ(duplicated.get_world_handle(), parent.get_world_handle());
}
*/

TEST(NamingAndIdentity, EntityNamingAndEquality) {
    world w = world::new_world_instance();
    entity e1 = w.add_entity();
    entity e2 = w.add_entity();

    e1.name() = "Player";
    EXPECT_EQ(e1.name(), "Player");

    EXPECT_NE(e1, e2);
    EXPECT_EQ(e1.get_uid(), static_cast<std::uint32_t>(e1));
}
/*
TEST(EcsSystems, AddSystemAndFilterRequireBoth) {
    // Create a new world and three entities
    world w = world::new_world_instance();

    auto e1 = w.add_entity();
    e1.add<Position>(0.0f, 0.0f);
    e1.add<Velocity>(1.0f, 1.0f);

    auto e2 = w.add_entity();
    e2.add<Position>(2.0f, 2.0f);
    // e2 has Position only

    auto e3 = w.add_entity();
    e3.add<Position>(3.0f, 3.0f);
    e3.add<Velocity>(2.0f, 2.0f);

    // Capture entities processed by the system
    std::vector<entity> processed;

    // Add a system that runs each frame and collects entities
    // having both Position and Velocity
    auto sysHandle = w.add_system(
        [&processed](world& wrld, float) {
            auto view = wrld.filter_entities<Position, Velocity>();
            for (auto ent : view) {
                processed.push_back(ent);
            }
        }
    );

    // Run one update to invoke the system
    w.update(1.0f / 60.0f);

    // Only e1 and e3 have both Position and Velocity
    ASSERT_EQ(processed.size(), 2u);
    EXPECT_NE(std::find(processed.begin(), processed.end(), e1), processed.end());
    EXPECT_NE(std::find(processed.begin(), processed.end(), e3), processed.end());
}
*/
TEST(EcsFilter, DirectFilterRequireExclude) {
    // Create world and entities
    world w = world::new_world_instance();

    auto a = w.add_entity();
    a.add<Position>(1.f, 1.f);
    a.add<Velocity>(1.f, 0.f);

    auto b = w.add_entity();
    b.add<Position>(2.f, 2.f);
    // b has Position only

    auto c = w.add_entity();
    c.add<Velocity>(0.f, 1.f);
    // c has Velocity only

    // Filter for entities that have Position, but exclude those with Velocity
    auto onlyPosition = w.filter_entities<Position>(exclude<Velocity>);

    // Expect exactly one entity: b
    ASSERT_EQ(onlyPosition.size(), 1u);
    EXPECT_EQ(onlyPosition.front(), b);
}
