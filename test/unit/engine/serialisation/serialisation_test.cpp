// world_serialization_tests.cpp

#include <gtest/gtest.h>
#include <filesystem>
#include <Ecs/ecs.h>

using namespace ecs;

//-----------------------------------------------------------------------------
// Test Components
//-----------------------------------------------------------------------------
struct Position { float x, y; };
struct Velocity { float dx, dy; };

//-----------------------------------------------------------------------------
// Test Fixture for World Serialization
//-----------------------------------------------------------------------------
class WorldSerializationTests : public ::testing::Test {
protected:
    std::filesystem::path yaml_file;
    std::filesystem::path bin_file;

    void SetUp() override {
        // Prepare temp file paths
        auto tmp = std::filesystem::temp_directory_path();
        yaml_file = tmp / "world_test.yaml";
        bin_file = tmp / "world_test.bin";

        // Clear reflection registry
        ReflectionRegistry::Reset();

        // Register Position for reflection & binary snapshot
        RegisterReflectionComponent<Position>(
            "Position",
            MemberRegistrationV<&Position::x, "x">,
            MemberRegistrationV<&Position::y, "y">
        );

        // Register Velocity for reflection & binary snapshot
        RegisterReflectionComponent<Velocity>(
            "Velocity",
            MemberRegistrationV<&Velocity::dx, "dx">,
            MemberRegistrationV<&Velocity::dy, "dy">
        );


        auto r = ReflectionRegistry::Registry();
        r.m_FieldNames;
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove(yaml_file, ec);
        std::filesystem::remove(bin_file, ec);
    }
};

//-----------------------------------------------------------------------------
// Test: Saving and loading an empty world via YAML
//-----------------------------------------------------------------------------
TEST_F(WorldSerializationTests, EmptyWorldYaml) {
    world w1 = world::new_world_instance();
    w1.SaveYAML(yaml_file.string());

    world w2 = world::new_world_instance();
    w2.LoadYAML(yaml_file.string());

    // No Position or Velocity components should exist
    auto posView = w2.filter_entities<Position>();
    EXPECT_TRUE(posView.empty());

    auto velView = w2.filter_entities<Velocity>();
    EXPECT_TRUE(velView.empty());
}

//-----------------------------------------------------------------------------
// Test: Saving and loading a populated world via YAML
//-----------------------------------------------------------------------------
TEST_F(WorldSerializationTests, YamlSaveLoadPreservesEntitiesAndComponents) {
    // Create and populate world
    world w1 = world::new_world_instance();
    auto e1 = w1.add_entity();
    e1.add<Position>(1.0f, 2.0f);
    e1.add<Velocity>(0.5f, 0.75f);

    auto e2 = w1.add_entity();
    e2.add<Position>(3.0f, 4.0f);
    // e2 has no Velocity

    // Serialize to YAML
    w1.SaveYAML(yaml_file.string());

    // Deserialize into new world
    world w2 = world::new_world_instance();
    w2.LoadYAML(yaml_file.string());

    // Only e1 should appear in Position+Velocity view
    auto bothView = w2.filter_entities<Position, Velocity>();
    ASSERT_EQ(bothView.size(), 1u);

    auto loadedE = bothView.front();
    auto pos = loadedE.get<Position>();
    auto vel = loadedE.get<Velocity>();
    EXPECT_FLOAT_EQ(pos.x, 1.0f);
    EXPECT_FLOAT_EQ(pos.y, 2.0f);
    EXPECT_FLOAT_EQ(vel.dx, 0.5f);
    EXPECT_FLOAT_EQ(vel.dy, 0.75f);

    // e2 should appear in Position-only view
    auto posOnly = w2.filter_entities<Position>();
    EXPECT_EQ(posOnly.size(), 2u);
}

//-----------------------------------------------------------------------------
// Test: Saving and loading an empty world via binary snapshot
//-----------------------------------------------------------------------------
TEST_F(WorldSerializationTests, EmptyWorldBinary) {
    world w1 = world::new_world_instance();
    w1.serialise_world_bin(bin_file.string());

    world w2 = world::new_world_instance();
    w2.deserialise_world_bin(bin_file.string());

    EXPECT_TRUE(w2.filter_entities<Position>().empty());
    EXPECT_TRUE(w2.filter_entities<Velocity>().empty());
}

//-----------------------------------------------------------------------------
// Test: Saving and loading a populated world via binary snapshot
//-----------------------------------------------------------------------------
TEST_F(WorldSerializationTests, BinaryPreservesEntitiesAndComponents) {
    // Create and populate world
    world w1 = world::new_world_instance();
    auto e1 = w1.add_entity();
    e1.add<Position>(5.0f, 6.0f);
    e1.add<Velocity>(1.5f, 2.5f);

    auto e2 = w1.add_entity();
    e2.add<Velocity>(0.1f, 0.2f);
    // e2 has no Position
    auto r = ReflectionRegistry::Registry();
    // Serialize to binary
    w1.serialise_world_bin(bin_file.string());

    // Deserialize into new world
    world w2 = world::new_world_instance();
    w2.deserialise_world_bin(bin_file.string());

    // Only e1 should appear in Position+Velocity view
    auto bothView = w2.filter_entities<Position, Velocity>();
    ASSERT_EQ(bothView.size(), 1u);

    auto loadedE = bothView.front();
    auto pos = loadedE.get<Position>();
    auto vel = loadedE.get<Velocity>();
    EXPECT_FLOAT_EQ(pos.x, 5.0f);
    EXPECT_FLOAT_EQ(pos.y, 6.0f);
    EXPECT_FLOAT_EQ(vel.dx, 1.5f);
    EXPECT_FLOAT_EQ(vel.dy, 2.5f);

    // e2 should appear in Velocity-only view
    auto velOnly = w2.filter_entities<Velocity>();
    EXPECT_EQ(velOnly.size(), 2u);
}
