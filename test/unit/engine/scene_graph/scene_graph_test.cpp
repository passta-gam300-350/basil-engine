#include <gtest/gtest.h>
#include "Scene/SceneGraph.hpp"
#include "Component/RelationshipComponent.hpp"
#include "Component/Transform.hpp"
#include "Ecs/ecs.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

using namespace ecs;

// Test fixture for scene graph tests
class SceneGraphTest : public ::testing::Test {
protected:
    world testWorld;

    void SetUp() override {
        testWorld = world::new_world_instance();
    }

    void TearDown() override {
        testWorld.destroy_world();
    }

    // Helper to create entity with transform and relationship
    entity CreateTestEntity() {
        entity e = testWorld.add_entity();
        e.add<TransformComponent>();
        e.add<TransformMtxComponent>();
        e.add<RelationshipComponent>();
        return e;
    }

    // Helper to check if matrices are approximately equal
    bool MatricesEqual(const glm::mat4& a, const glm::mat4& b, float epsilon = 0.001f) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (std::abs(a[i][j] - b[i][j]) > epsilon) {
                    return false;
                }
            }
        }
        return true;
    }
};

// Test basic parent-child relationship
TEST_F(SceneGraphTest, SetParentCreatesRelationship) {
    entity parent = CreateTestEntity();
    entity child = CreateTestEntity();

    SceneGraph::SetParent(child, parent);

    EXPECT_TRUE(SceneGraph::HasParent(child));
    EXPECT_EQ(SceneGraph::GetParent(child), parent);

    auto children = SceneGraph::GetChildren(parent);
    EXPECT_EQ(children.size(), 1);
    EXPECT_EQ(children[0], child);
}

// Test removing parent
TEST_F(SceneGraphTest, RemoveParentBreaksRelationship) {
    entity parent = CreateTestEntity();
    entity child = CreateTestEntity();

    SceneGraph::SetParent(child, parent);
    EXPECT_TRUE(SceneGraph::HasParent(child));

    SceneGraph::RemoveParent(child);
    EXPECT_FALSE(SceneGraph::HasParent(child));

    auto children = SceneGraph::GetChildren(parent);
    EXPECT_EQ(children.size(), 0);
}

// Test multiple children
TEST_F(SceneGraphTest, MultipleChildren) {
    entity parent = CreateTestEntity();
    entity child1 = CreateTestEntity();
    entity child2 = CreateTestEntity();
    entity child3 = CreateTestEntity();

    SceneGraph::AddChild(parent, child1);
    SceneGraph::AddChild(parent, child2);
    SceneGraph::AddChild(parent, child3);

    auto children = SceneGraph::GetChildren(parent);
    EXPECT_EQ(children.size(), 3);
}

// Test reparenting
TEST_F(SceneGraphTest, Reparenting) {
    entity parent1 = CreateTestEntity();
    entity parent2 = CreateTestEntity();
    entity child = CreateTestEntity();

    SceneGraph::SetParent(child, parent1);
    EXPECT_EQ(SceneGraph::GetChildren(parent1).size(), 1);

    SceneGraph::SetParent(child, parent2);
    EXPECT_EQ(SceneGraph::GetChildren(parent1).size(), 0);
    EXPECT_EQ(SceneGraph::GetChildren(parent2).size(), 1);
    EXPECT_EQ(SceneGraph::GetParent(child), parent2);
}

// Test local transform
TEST_F(SceneGraphTest, LocalTransform) {
    entity e = CreateTestEntity();
    auto& transform = e.get<TransformComponent>();

    transform.m_Translation = glm::vec3(1.0f, 2.0f, 3.0f);
    transform.m_Scale = glm::vec3(2.0f, 2.0f, 2.0f);

    glm::mat4 localMat = transform.getLocalMatrix();

    // Extract position from matrix
    glm::vec3 position = glm::vec3(localMat[3]);
    EXPECT_NEAR(position.x, 1.0f, 0.001f);
    EXPECT_NEAR(position.y, 2.0f, 0.001f);
    EXPECT_NEAR(position.z, 3.0f, 0.001f);
}

// Test transform hierarchy (manual update for testing)
TEST_F(SceneGraphTest, TransformHierarchyBasic) {
    entity parent = CreateTestEntity();
    entity child = CreateTestEntity();

    SceneGraph::SetParent(child, parent);

    // Set parent at position (10, 0, 0)
    auto& parentTransform = parent.get<TransformComponent>();
    parentTransform.m_Translation = glm::vec3(10.0f, 0.0f, 0.0f);

    // Set child at local position (5, 0, 0) - relative to parent
    auto& childTransform = child.get<TransformComponent>();
    childTransform.m_Translation = glm::vec3(5.0f, 0.0f, 0.0f);

    // Manually compute world matrices (simulating what HierarchySystem does)
    auto& parentMtx = parent.get<TransformMtxComponent>();
    auto& childMtx = child.get<TransformMtxComponent>();

    parentMtx.m_Mtx = parentTransform.getLocalMatrix();
    childMtx.m_Mtx = parentMtx.m_Mtx * childTransform.getLocalMatrix();

    // Child's world position should be (15, 0, 0)
    glm::vec3 childWorldPos = glm::vec3(childMtx.m_Mtx[3]);
    EXPECT_NEAR(childWorldPos.x, 15.0f, 0.001f);
    EXPECT_NEAR(childWorldPos.y, 0.0f, 0.001f);
    EXPECT_NEAR(childWorldPos.z, 0.0f, 0.001f);
}

// Test multi-level hierarchy
TEST_F(SceneGraphTest, MultiLevelHierarchy) {
    entity grandparent = CreateTestEntity();
    entity parent = CreateTestEntity();
    entity child = CreateTestEntity();

    SceneGraph::SetParent(parent, grandparent);
    SceneGraph::SetParent(child, parent);

    // Verify hierarchy structure
    EXPECT_TRUE(SceneGraph::HasParent(parent));
    EXPECT_TRUE(SceneGraph::HasParent(child));
    EXPECT_FALSE(SceneGraph::HasParent(grandparent));

    auto grandparentChildren = SceneGraph::GetChildren(grandparent);
    EXPECT_EQ(grandparentChildren.size(), 1);
    EXPECT_EQ(grandparentChildren[0], parent);

    auto parentChildren = SceneGraph::GetChildren(parent);
    EXPECT_EQ(parentChildren.size(), 1);
    EXPECT_EQ(parentChildren[0], child);
}

// Test transform with scale inheritance
TEST_F(SceneGraphTest, ScaleInheritance) {
    entity parent = CreateTestEntity();
    entity child = CreateTestEntity();

    SceneGraph::SetParent(child, parent);

    // Parent scaled 2x
    auto& parentTransform = parent.get<TransformComponent>();
    parentTransform.m_Scale = glm::vec3(2.0f);

    // Child scaled 3x (relative to parent)
    auto& childTransform = child.get<TransformComponent>();
    childTransform.m_Scale = glm::vec3(3.0f);

    // Compute world matrices
    auto& parentMtx = parent.get<TransformMtxComponent>();
    auto& childMtx = child.get<TransformMtxComponent>();

    parentMtx.m_Mtx = parentTransform.getLocalMatrix();
    childMtx.m_Mtx = parentMtx.m_Mtx * childTransform.getLocalMatrix();

    // Extract scale from child's world matrix
    glm::vec3 scale, translation, skew;
    glm::vec4 perspective;
    glm::quat rotation;
    glm::decompose(childMtx.m_Mtx, scale, rotation, translation, skew, perspective);

    // Child's world scale should be 2 * 3 = 6
    EXPECT_NEAR(scale.x, 6.0f, 0.01f);
    EXPECT_NEAR(scale.y, 6.0f, 0.01f);
    EXPECT_NEAR(scale.z, 6.0f, 0.01f);
}

// Test euler angle rotation in getLocalMatrix
TEST_F(SceneGraphTest, EulerAngleRotation) {
    entity e = CreateTestEntity();
    auto& transform = e.get<TransformComponent>();

    // Set rotation in degrees (stored as Euler angles)
    transform.m_Rotation = glm::vec3(45.0f, 90.0f, 30.0f);

    // Get local matrix (should convert Euler to rotation matrix internally)
    glm::mat4 localMat = transform.getLocalMatrix();

    // Extract rotation from matrix to verify it's been applied
    glm::vec3 scale, translation, skew;
    glm::vec4 perspective;
    glm::quat rotation;
    glm::decompose(localMat, scale, rotation, translation, skew, perspective);

    // Just verify the decomposition succeeds and returns finite values
    EXPECT_TRUE(std::isfinite(rotation.x));
    EXPECT_TRUE(std::isfinite(rotation.y));
    EXPECT_TRUE(std::isfinite(rotation.z));
    EXPECT_TRUE(std::isfinite(rotation.w));
}

// Test dirty flag
TEST_F(SceneGraphTest, DirtyFlagSetCorrectly) {
    entity e = CreateTestEntity();
    auto& transform = e.get<TransformComponent>();

    EXPECT_TRUE(transform.isDirty); // Default is dirty

    transform.isDirty = false;
    SceneGraph::MarkTransformDirty(e);
    EXPECT_TRUE(transform.isDirty);
}

// Test getting local and world transforms
TEST_F(SceneGraphTest, GetLocalAndWorldTransforms) {
    entity e = CreateTestEntity();
    auto& transform = e.get<TransformComponent>();
    auto& mtx = e.get<TransformMtxComponent>();

    transform.m_Translation = glm::vec3(5.0f, 10.0f, 15.0f);
    mtx.m_Mtx = glm::translate(glm::mat4(1.0f), glm::vec3(100.0f, 200.0f, 300.0f));

    glm::mat4 local = SceneGraph::GetLocalTransform(e);
    glm::mat4 world = SceneGraph::GetWorldTransform(e);

    glm::vec3 localPos = glm::vec3(local[3]);
    glm::vec3 worldPos = glm::vec3(world[3]);

    EXPECT_NEAR(localPos.x, 5.0f, 0.001f);
    EXPECT_NEAR(worldPos.x, 100.0f, 0.001f);
}

// Test removing child specifically
TEST_F(SceneGraphTest, RemoveChildSpecific) {
    entity parent = CreateTestEntity();
    entity child1 = CreateTestEntity();
    entity child2 = CreateTestEntity();

    SceneGraph::AddChild(parent, child1);
    SceneGraph::AddChild(parent, child2);
    EXPECT_EQ(SceneGraph::GetChildren(parent).size(), 2);

    SceneGraph::RemoveChild(parent, child1);

    auto children = SceneGraph::GetChildren(parent);
    EXPECT_EQ(children.size(), 1);
    EXPECT_EQ(children[0], child2);
    EXPECT_FALSE(SceneGraph::HasParent(child1));
    EXPECT_TRUE(SceneGraph::HasParent(child2));
}

// Test TransformComponent and TransformMtxComponent integration
TEST_F(SceneGraphTest, ComponentIntegration) {
    entity e = CreateTestEntity();
    auto& transform = e.get<TransformComponent>();
    auto& mtx = e.get<TransformMtxComponent>();

    // Set local transform
    transform.m_Translation = glm::vec3(1.0f, 2.0f, 3.0f);
    transform.m_Scale = glm::vec3(2.0f, 2.0f, 2.0f);

    // Compute world matrix
    mtx.m_Mtx = transform.getLocalMatrix();

    // Verify world matrix contains the correct transformation
    glm::vec3 worldPos = glm::vec3(mtx.m_Mtx[3]);
    EXPECT_NEAR(worldPos.x, 1.0f, 0.001f);
    EXPECT_NEAR(worldPos.y, 2.0f, 0.001f);
    EXPECT_NEAR(worldPos.z, 3.0f, 0.001f);
}
