// reflection_tests.cpp

#include <gtest/gtest.h>
#include <entt/entt.hpp>
#include "Ecs/internal/reflection.h"    
#include <yaml-cpp/yaml.h>

#include <unordered_map>
#include <variant>
#include <string>

/*
//-----------------------------------------------------------------------------
// Helper Node for (De)Serialization Tests
//-----------------------------------------------------------------------------
struct MapNode {
    using Value = std::variant<int, float, double, std::string, MapNode>;
    std::unordered_map<std::string, Value> data;

    Value& operator[](const std::string& key) {
        return data[key];
    }

    bool has(const std::string& key) const {
        return data.find(key) != data.end();
    }
};*/

using MapNode = YAML::Node;

//-----------------------------------------------------------------------------
// Fixture to Clear the ReflectionRegistry Between Tests
//-----------------------------------------------------------------------------
struct ReflectionFixture : ::testing::Test {
protected:
    void SetUp() override {
        ReflectionRegistry::Reset();
    }
};


//-----------------------------------------------------------------------------
// Suite 1: TypeNameTests
//-----------------------------------------------------------------------------
TEST(TypeNameTests, LiteralVsFunction) {
    auto a = ToTypeName("MyType");
    auto b = "MyType"_tn;
    EXPECT_EQ(a, b);
}

TEST(TypeNameTests, DifferentStringsDifferentHashes) {
    EXPECT_NE("Alpha"_tn, "Beta"_tn);
}

//-----------------------------------------------------------------------------
// Suite 2: ReflectionRegistryTests
//-----------------------------------------------------------------------------
TEST_F(ReflectionFixture, Registry_EmptyOnStartup) {
    EXPECT_TRUE(ReflectionRegistry::types().empty());
    EXPECT_TRUE(ReflectionRegistry::BinSerializerRegistryInstance().empty());
}

TEST_F(ReflectionFixture, RegisterReflectionComponentPopulatesRegistry) {
    struct Demo { int x; double y; };
    RegisterReflectionComponent<Demo>("Demo", MemberRegistrationV<&Demo::x, "x">, MemberRegistrationV<&Demo::y, "y">);
    auto& types = ReflectionRegistry::types();
    auto id = entt::type_hash<Demo>::value();
    EXPECT_EQ(types.count(id), 1u);
}

/*
TEST_F(ReflectionFixture, ReflectComponentMacroAppendsSerializer) {
    struct Demo2 { int y; };
    //RegisterReflectionComponent<Demo2>("Demo2", &Demo2::y, "x"_tn);
    ReflectComponentLocal(Demo2, .data<&Demo2::y>("y"_tn));

    EXPECT_EQ(ReflectionRegistry::BinSerializerRegistryInstance().size(), 1u);
}*/

//-----------------------------------------------------------------------------
// Suite 3: SerializeTypeTests
//-----------------------------------------------------------------------------
class SerializeTypeTests : public ReflectionFixture {
protected:
    struct Simple { int a; float b; };
    struct Nested { int x; Simple s; std::string name; };

    void SetUp() override {
        ReflectionFixture::SetUp();
        RegisterReflectionComponent<Simple>(
            "Simple",
            MemberRegistrationV<&Simple::a, "a">,
            MemberRegistrationV<&Simple::b, "b">
        );
        RegisterReflectionComponent<Nested>(
            "Nested",
            MemberRegistrationV<&Nested::x, "x">,
            MemberRegistrationV<&Nested::s, "s">,
            MemberRegistrationV<&Nested::name, "name">
        );
    }
};

TEST_F(SerializeTypeTests, SerializePrimitiveMembers) {
    Simple obj{ 42, 3.14f };
    auto any = ResolveType("Simple"_tn).from_void((void*)(&obj));

    MapNode out;
    SerializeType(any, out);

    EXPECT_TRUE(out["a"]);
    EXPECT_TRUE(out["b"]);
    EXPECT_EQ(out["a"].as<int>(), 42);
    EXPECT_FLOAT_EQ(out["b"].as<float>(), 3.14f);
}

TEST_F(SerializeTypeTests, SerializeNestedMember) {
    Nested obj{ 7, {1, 2.5f}, "hello" };
    auto any = ResolveType("Nested"_tn).from_void((void*)(&obj));

    MapNode out;
    SerializeType(any, out);

    ASSERT_TRUE(out["s"]);
    auto sNode = out["s"];
    EXPECT_EQ(sNode["a"].as<int>(), 1);
    EXPECT_FLOAT_EQ(sNode["b"].as<float>(), 2.5f);

    EXPECT_EQ(out["name"].as<std::string>(), "hello");
}

//-----------------------------------------------------------------------------
// Suite 4: SerializeEntityTests
//-----------------------------------------------------------------------------
class SerializeEntityTests : public ReflectionFixture {
protected:
    struct Simple { int a; float b; };

    void SetUp() override {
        ReflectionFixture::SetUp();
        RegisterReflectionComponent<Simple>(
            "Simple",
            MemberRegistrationV<&Simple::a, "a">,
            MemberRegistrationV<&Simple::b, "b">
        );
    }
};

TEST_F(SerializeEntityTests, EntitySerializesRegisteredComponents) {
    entt::registry reg;
    auto e = reg.create();
    reg.emplace<Simple>(e, 9, 9.5f);

    // Reflect the type into the registry
    ReflectionRegistry::types()[entt::type_hash<Simple>::value()]
        = entt::resolve<Simple>();

    MapNode out = SerializeEntity<MapNode>(reg, e);

    ASSERT_TRUE(out["Simple"]);
    auto compNode = out["Simple"];
    EXPECT_EQ(compNode["a"].as<int>(), 9);
    EXPECT_FLOAT_EQ(compNode["b"].as<float>(), 9.5f);
}

//-----------------------------------------------------------------------------
// Suite 5: DeserializeTypeTests
//-----------------------------------------------------------------------------
class DeserializeTypeTests : public ReflectionFixture {
protected:
    struct Simple { int a; float b; };
    struct Nested { int x; Simple s; std::string name; };

    void SetUp() override {
        ReflectionFixture::SetUp();
        RegisterReflectionComponent<Simple>(
            "Simple",
            MemberRegistrationV<&Simple::a, "a">,
            MemberRegistrationV<&Simple::b, "b">
        );
        RegisterReflectionComponent<Nested>(
            "Nested",
            MemberRegistrationV<&Nested::x, "x">,
            MemberRegistrationV<&Nested::s, "s">,
            MemberRegistrationV<&Nested::name, "name">
        );
    }
};

TEST_F(DeserializeTypeTests, PrimitiveDeserialization) {
    Simple obj{};
    auto any = ResolveType("Simple"_tn).from_void((void*)(&obj));

    MapNode in;
    in["a"] = 123;
    in["b"] = 4.56f;

    DeserializeType(in, any);
    EXPECT_EQ(obj.a, 123);
    EXPECT_FLOAT_EQ(obj.b, 4.56f);
}

TEST_F(DeserializeTypeTests, NestedDeserialization) {
    Nested obj{};
    auto any = ResolveType("Nested"_tn).from_void((void*)(&obj));

    MapNode in;
    in["x"] = 8;

    MapNode sNode;
    sNode["a"] = 5;
    sNode["b"] = 7.7f;
    in["s"] = sNode;

    in["name"] = std::string("bob");

    DeserializeType(in, any);

    EXPECT_EQ(obj.x, 8);
    EXPECT_EQ(obj.s.a, 5);
    EXPECT_FLOAT_EQ(obj.s.b, 7.7f);
    EXPECT_EQ(obj.name, "bob");
}

//-----------------------------------------------------------------------------
// Suite 6: DeserializeEntityTests
//-----------------------------------------------------------------------------
class DeserializeEntityTests : public ReflectionFixture {
protected:
    struct Simple { int a; float b; };

    void SetUp() override {
        ReflectionFixture::SetUp();
        RegisterReflectionComponent<Simple>(
            "Simple",
            MemberRegistrationV<&Simple::a, "a">,
            MemberRegistrationV<&Simple::b, "b">
        );
    }
};

TEST_F(DeserializeEntityTests, EntityDeserializationAddsEntityAndComponent) {
    entt::registry reg;
    MapNode entityNode;
    MapNode comps;
    MapNode simpleNode;
    simpleNode["a"] = 77;
    simpleNode["b"] = 7.7f;
    comps["Simple"] = simpleNode;
    //entityNode["Entity"] = comps;
    entityNode = comps;

    DeserializeEntity<MapNode>(reg, entityNode);
    
    auto view = reg.view<typename DeserializeEntityTests::Simple>();
    ASSERT_EQ(view.size(), 1u);

    auto e = *view.begin();
    auto& c = view.get<typename DeserializeEntityTests::Simple>(e);
    EXPECT_EQ(c.a, 77);
    EXPECT_FLOAT_EQ(c.b, 7.7f);
}
/*
//-----------------------------------------------------------------------------
// Suite 7: MacroRegistrationTests
//-----------------------------------------------------------------------------
TEST_F(ReflectionFixture, MacroRegistrationCreatesMetaTypeAndSerializer) {
    struct M { int v; };
    ReflectComponent(M, .data<&M::v>("v"_tn));

    auto& types = ReflectionRegistry::types();
    auto id = entt::type_hash<M>::value();
    EXPECT_EQ(types.count(id), 1u);

    EXPECT_FALSE(ReflectionRegistry::BinSerializerRegistryInstance().empty());
}
*/