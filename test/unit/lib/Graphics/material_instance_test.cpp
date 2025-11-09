/******************************************************************************/
/*!
\file   material_instance_test.cpp
\author Team PASSTA
\date   2025/10/20
\brief  Unit tests for Material Instance System

Tests cover:
- MaterialInstance: Property overrides, fallback behavior, cleanup
- MaterialInstanceManager: Instance lifecycle, lookup, memory management

Copyright (C) 2025 DigiPen Institute of Technology.
*/
/******************************************************************************/

#include <gtest/gtest.h>
#include <Resources/Material.h>
#include <Resources/MaterialInstance.h>
#include <Resources/MaterialInstanceManager.h>
#include <Resources/Shader.h>
#include <Resources/Texture.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

// ============================================================================
// Test Fixtures
// ============================================================================

class MaterialInstanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple shader for testing
        // Note: This is a minimal setup - real shaders require OpenGL context
        baseMaterial = std::make_shared<Material>(nullptr, "TestMaterial");

        // Set some base properties
        baseMaterial->SetFloat("u_Roughness", 0.5f);
        baseMaterial->SetVec3("u_Albedo", glm::vec3(0.8f, 0.8f, 0.8f));
        baseMaterial->SetVec4("u_Emissive", glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        baseMaterial->SetMat4("u_Transform", glm::mat4(1.0f));
    }

    void TearDown() override {
        baseMaterial.reset();
    }

    std::shared_ptr<Material> baseMaterial;
};

class MaterialInstanceManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<MaterialInstanceManager>();
        baseMaterial = std::make_shared<Material>(nullptr, "TestMaterial");
        baseMaterial->SetFloat("u_Roughness", 0.5f);
        baseMaterial->SetVec3("u_Albedo", glm::vec3(0.8f, 0.8f, 0.8f));
    }

    void TearDown() override {
        manager->ClearAllInstances();
        manager.reset();
        baseMaterial.reset();
    }

    std::unique_ptr<MaterialInstanceManager> manager;
    std::shared_ptr<Material> baseMaterial;
};

// ============================================================================
// MaterialInstance Tests
// ============================================================================

TEST_F(MaterialInstanceTest, Construction) {
    ASSERT_NE(baseMaterial, nullptr);

    MaterialInstance instance(baseMaterial);

    EXPECT_EQ(instance.GetBaseMaterial(), baseMaterial);
    EXPECT_EQ(instance.GetShader(), nullptr); // No shader in base
    EXPECT_FALSE(instance.HasOverrides());
}

TEST_F(MaterialInstanceTest, ConstructionWithNullThrows) {
    EXPECT_THROW(MaterialInstance instance(nullptr), std::invalid_argument);
}

TEST_F(MaterialInstanceTest, FloatPropertyOverride) {
    MaterialInstance instance(baseMaterial);

    // Initially should return base material's value
    EXPECT_FLOAT_EQ(instance.GetFloat("u_Roughness", 0.0f), 0.5f);
    EXPECT_FALSE(instance.IsPropertyOverridden("u_Roughness"));

    // Set override
    instance.SetFloat("u_Roughness", 0.2f);

    // Should now return overridden value
    EXPECT_FLOAT_EQ(instance.GetFloat("u_Roughness", 0.0f), 0.2f);
    EXPECT_TRUE(instance.IsPropertyOverridden("u_Roughness"));
    EXPECT_TRUE(instance.HasOverrides());

    // Base material should be unchanged
    EXPECT_FLOAT_EQ(baseMaterial->GetFloatProperty("u_Roughness", 0.0f), 0.5f);
}

TEST_F(MaterialInstanceTest, Vec3PropertyOverride) {
    MaterialInstance instance(baseMaterial);

    // Initially should return base material's value
    glm::vec3 baseAlbedo = instance.GetVec3("u_Albedo", glm::vec3(0.0f));
    EXPECT_FLOAT_EQ(baseAlbedo.x, 0.8f);
    EXPECT_FLOAT_EQ(baseAlbedo.y, 0.8f);
    EXPECT_FLOAT_EQ(baseAlbedo.z, 0.8f);

    // Set override
    glm::vec3 redColor(1.0f, 0.0f, 0.0f);
    instance.SetVec3("u_Albedo", redColor);

    // Should now return overridden value
    glm::vec3 overriddenAlbedo = instance.GetVec3("u_Albedo", glm::vec3(0.0f));
    EXPECT_FLOAT_EQ(overriddenAlbedo.x, 1.0f);
    EXPECT_FLOAT_EQ(overriddenAlbedo.y, 0.0f);
    EXPECT_FLOAT_EQ(overriddenAlbedo.z, 0.0f);
    EXPECT_TRUE(instance.IsPropertyOverridden("u_Albedo"));
}

TEST_F(MaterialInstanceTest, Vec4PropertyOverride) {
    MaterialInstance instance(baseMaterial);

    // Set override
    glm::vec4 emissive(1.0f, 0.5f, 0.0f, 0.8f);
    instance.SetVec4("u_Emissive", emissive);

    // Verify override
    glm::vec4 result = instance.GetVec4("u_Emissive", glm::vec4(0.0f));
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 0.5f);
    EXPECT_FLOAT_EQ(result.z, 0.0f);
    EXPECT_FLOAT_EQ(result.w, 0.8f);
    EXPECT_TRUE(instance.IsPropertyOverridden("u_Emissive"));
}

TEST_F(MaterialInstanceTest, Mat4PropertyOverride) {
    MaterialInstance instance(baseMaterial);

    // Set override
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));
    instance.SetMat4("u_Transform", transform);

    // Verify override
    glm::mat4 result = instance.GetMat4("u_Transform", glm::mat4(1.0f));
    EXPECT_FLOAT_EQ(result[3][0], 1.0f);
    EXPECT_FLOAT_EQ(result[3][1], 2.0f);
    EXPECT_FLOAT_EQ(result[3][2], 3.0f);
    EXPECT_TRUE(instance.IsPropertyOverridden("u_Transform"));
}

TEST_F(MaterialInstanceTest, MultipleOverrides) {
    MaterialInstance instance(baseMaterial);

    // Override multiple properties
    instance.SetFloat("u_Roughness", 0.1f);
    instance.SetFloat("u_Metallic", 0.9f);
    instance.SetVec3("u_Albedo", glm::vec3(1.0f, 0.0f, 0.0f));

    EXPECT_TRUE(instance.HasOverrides());
    EXPECT_TRUE(instance.IsPropertyOverridden("u_Roughness"));
    EXPECT_TRUE(instance.IsPropertyOverridden("u_Metallic"));
    EXPECT_TRUE(instance.IsPropertyOverridden("u_Albedo"));

    // Verify values
    EXPECT_FLOAT_EQ(instance.GetFloat("u_Roughness", 0.0f), 0.1f);
    EXPECT_FLOAT_EQ(instance.GetFloat("u_Metallic", 0.0f), 0.9f);
    glm::vec3 albedo = instance.GetVec3("u_Albedo", glm::vec3(0.0f));
    EXPECT_FLOAT_EQ(albedo.x, 1.0f);
}

TEST_F(MaterialInstanceTest, FallbackToBaseMaterial) {
    MaterialInstance instance(baseMaterial);

    // Property not overridden should fall back to base
    EXPECT_FLOAT_EQ(instance.GetFloat("u_Roughness", 0.0f), 0.5f);

    // Non-existent property should return default
    EXPECT_FLOAT_EQ(instance.GetFloat("u_NonExistent", 99.0f), 99.0f);
}

TEST_F(MaterialInstanceTest, ClearOverrides) {
    MaterialInstance instance(baseMaterial);

    // Set some overrides
    instance.SetFloat("u_Roughness", 0.1f);
    instance.SetVec3("u_Albedo", glm::vec3(1.0f, 0.0f, 0.0f));
    EXPECT_TRUE(instance.HasOverrides());

    // Clear all overrides
    instance.ClearOverrides();

    // Should have no overrides and fall back to base
    EXPECT_FALSE(instance.HasOverrides());
    EXPECT_FALSE(instance.IsPropertyOverridden("u_Roughness"));
    EXPECT_FALSE(instance.IsPropertyOverridden("u_Albedo"));
    EXPECT_FLOAT_EQ(instance.GetFloat("u_Roughness", 0.0f), 0.5f); // Back to base value
}

TEST_F(MaterialInstanceTest, InstanceNameGeneration) {
    MaterialInstance instance(baseMaterial);
    EXPECT_EQ(instance.GetName(), "TestMaterial (Instance)");

    MaterialInstance namedInstance(baseMaterial, "CustomInstance");
    EXPECT_EQ(namedInstance.GetName(), "CustomInstance");
}

// ============================================================================
// MaterialInstanceManager Tests
// ============================================================================

TEST_F(MaterialInstanceManagerTest, Construction) {
    EXPECT_EQ(manager->GetInstanceCount(), 0);
}

TEST_F(MaterialInstanceManagerTest, GetSharedMaterial) {
    uint64_t objectID = 123;

    auto sharedMat = manager->GetSharedMaterial(baseMaterial);

    EXPECT_EQ(sharedMat, baseMaterial);
    EXPECT_FALSE(manager->HasInstance(objectID)); // Should not create instance
}

TEST_F(MaterialInstanceManagerTest, CreateInstance) {
    uint64_t objectID = 456;

    EXPECT_FALSE(manager->HasInstance(objectID));

    auto instance = manager->GetMaterialInstance(objectID, baseMaterial);

    ASSERT_NE(instance, nullptr);
    EXPECT_TRUE(manager->HasInstance(objectID));
    EXPECT_EQ(manager->GetInstanceCount(), 1);
    EXPECT_EQ(instance->GetBaseMaterial(), baseMaterial);
}

TEST_F(MaterialInstanceManagerTest, GetMaterialInstanceWithNullMaterial) {
    uint64_t objectID = 789;

    auto instance = manager->GetMaterialInstance(objectID, nullptr);

    EXPECT_EQ(instance, nullptr);
    EXPECT_FALSE(manager->HasInstance(objectID));
}

TEST_F(MaterialInstanceManagerTest, ReuseExistingInstance) {
    uint64_t objectID = 111;

    // Create instance
    auto instance1 = manager->GetMaterialInstance(objectID, baseMaterial);
    ASSERT_NE(instance1, nullptr);

    // Modify instance
    instance1->SetFloat("u_Roughness", 0.3f);

    // Get instance again (should return same instance)
    auto instance2 = manager->GetMaterialInstance(objectID, baseMaterial);

    EXPECT_EQ(instance1, instance2); // Same pointer
    EXPECT_FLOAT_EQ(instance2->GetFloat("u_Roughness", 0.0f), 0.3f); // Preserves modifications
    EXPECT_EQ(manager->GetInstanceCount(), 1); // Still only one instance
}

TEST_F(MaterialInstanceManagerTest, MultipleObjectInstances) {
    uint64_t obj1 = 100;
    uint64_t obj2 = 200;
    uint64_t obj3 = 300;

    auto instance1 = manager->GetMaterialInstance(obj1, baseMaterial);
    auto instance2 = manager->GetMaterialInstance(obj2, baseMaterial);
    auto instance3 = manager->GetMaterialInstance(obj3, baseMaterial);

    EXPECT_EQ(manager->GetInstanceCount(), 3);
    EXPECT_NE(instance1, instance2);
    EXPECT_NE(instance2, instance3);
    EXPECT_TRUE(manager->HasInstance(obj1));
    EXPECT_TRUE(manager->HasInstance(obj2));
    EXPECT_TRUE(manager->HasInstance(obj3));
}

TEST_F(MaterialInstanceManagerTest, GetExistingInstance) {
    uint64_t objectID = 222;

    // No instance yet
    auto nonExistent = manager->GetExistingInstance(objectID);
    EXPECT_EQ(nonExistent, nullptr);

    // Create instance
    auto created = manager->GetMaterialInstance(objectID, baseMaterial);
    ASSERT_NE(created, nullptr);

    // Get existing
    auto existing = manager->GetExistingInstance(objectID);
    EXPECT_EQ(existing, created);
}

TEST_F(MaterialInstanceManagerTest, DestroyInstance) {
    uint64_t objectID = 333;

    // Create instance
    auto instance = manager->GetMaterialInstance(objectID, baseMaterial);
    EXPECT_TRUE(manager->HasInstance(objectID));
    EXPECT_EQ(manager->GetInstanceCount(), 1);

    // Destroy instance
    manager->DestroyInstance(objectID);

    EXPECT_FALSE(manager->HasInstance(objectID));
    EXPECT_EQ(manager->GetInstanceCount(), 0);
    EXPECT_EQ(manager->GetExistingInstance(objectID), nullptr);
}

TEST_F(MaterialInstanceManagerTest, DestroyNonExistentInstance) {
    uint64_t objectID = 444;

    // Should not crash
    EXPECT_NO_THROW(manager->DestroyInstance(objectID));
    EXPECT_FALSE(manager->HasInstance(objectID));
}

TEST_F(MaterialInstanceManagerTest, SetSharedMaterial) {
    uint64_t objectID = 555;

    // Create instance
    auto instance = manager->GetMaterialInstance(objectID, baseMaterial);
    instance->SetFloat("u_Roughness", 0.1f);
    EXPECT_TRUE(manager->HasInstance(objectID));

    // Create new base material
    auto newBaseMaterial = std::make_shared<Material>(nullptr, "NewMaterial");

    // Set shared material (should destroy instance)
    manager->SetSharedMaterial(objectID, newBaseMaterial);

    EXPECT_FALSE(manager->HasInstance(objectID));
    EXPECT_EQ(manager->GetInstanceCount(), 0);
}

TEST_F(MaterialInstanceManagerTest, ClearAllInstances) {
    // Create multiple instances
    manager->GetMaterialInstance(1, baseMaterial);
    manager->GetMaterialInstance(2, baseMaterial);
    manager->GetMaterialInstance(3, baseMaterial);

    EXPECT_EQ(manager->GetInstanceCount(), 3);

    // Clear all
    manager->ClearAllInstances();

    EXPECT_EQ(manager->GetInstanceCount(), 0);
    EXPECT_FALSE(manager->HasInstance(1));
    EXPECT_FALSE(manager->HasInstance(2));
    EXPECT_FALSE(manager->HasInstance(3));
}

TEST_F(MaterialInstanceManagerTest, IndependentInstances) {
    uint64_t obj1 = 1001;
    uint64_t obj2 = 1002;

    auto instance1 = manager->GetMaterialInstance(obj1, baseMaterial);
    auto instance2 = manager->GetMaterialInstance(obj2, baseMaterial);

    // Modify instance1
    instance1->SetFloat("u_Roughness", 0.1f);
    instance1->SetVec3("u_Albedo", glm::vec3(1.0f, 0.0f, 0.0f));

    // Modify instance2 differently
    instance2->SetFloat("u_Roughness", 0.9f);
    instance2->SetVec3("u_Albedo", glm::vec3(0.0f, 1.0f, 0.0f));

    // Verify independence
    EXPECT_FLOAT_EQ(instance1->GetFloat("u_Roughness", 0.0f), 0.1f);
    EXPECT_FLOAT_EQ(instance2->GetFloat("u_Roughness", 0.0f), 0.9f);

    glm::vec3 albedo1 = instance1->GetVec3("u_Albedo", glm::vec3(0.0f));
    glm::vec3 albedo2 = instance2->GetVec3("u_Albedo", glm::vec3(0.0f));

    EXPECT_FLOAT_EQ(albedo1.x, 1.0f); // Red
    EXPECT_FLOAT_EQ(albedo2.y, 1.0f); // Green

    // Base material unchanged
    EXPECT_FLOAT_EQ(baseMaterial->GetFloatProperty("u_Roughness", 0.0f), 0.5f);
}

// ============================================================================
// Texture API Tests
// ============================================================================

// Helper function to create a mock texture for testing
std::shared_ptr<Texture> CreateMockTexture(unsigned int id, const std::string& name) {
    auto texture = std::make_shared<Texture>();
    texture->id = id;
    texture->type = "diffuse";
    texture->path = name;
    texture->target = GL_TEXTURE_2D;
    return texture;
}

TEST_F(MaterialInstanceTest, TextureProperty) {
    auto texture = CreateMockTexture(100, "test_texture.png");

    // Set texture on base material
    baseMaterial->SetTexture("u_DiffuseMap", texture, 0);

    // Verify texture is stored
    EXPECT_TRUE(baseMaterial->HasTexture("u_DiffuseMap"));
    EXPECT_EQ(baseMaterial->GetTexture("u_DiffuseMap"), texture);
}

TEST_F(MaterialInstanceTest, TexturePropertyOverride) {
    auto baseTexture = CreateMockTexture(100, "base_texture.png");
    auto overrideTexture = CreateMockTexture(200, "override_texture.png");

    // Set texture on base material
    baseMaterial->SetTexture("u_DiffuseMap", baseTexture, 0);

    MaterialInstance instance(baseMaterial);

    // Initially should return base material's texture
    EXPECT_EQ(instance.GetTexture("u_DiffuseMap"), baseTexture);
    EXPECT_FALSE(instance.IsPropertyOverridden("u_DiffuseMap"));

    // Override texture
    instance.SetTexture("u_DiffuseMap", overrideTexture, 1);

    // Should now return overridden texture
    EXPECT_EQ(instance.GetTexture("u_DiffuseMap"), overrideTexture);
    EXPECT_TRUE(instance.IsPropertyOverridden("u_DiffuseMap"));
    EXPECT_TRUE(instance.HasOverrides());

    // Base material should be unchanged
    EXPECT_EQ(baseMaterial->GetTexture("u_DiffuseMap"), baseTexture);
}

TEST_F(MaterialInstanceTest, MultipleTextures) {
    auto diffuseTexture = CreateMockTexture(100, "diffuse.png");
    auto normalTexture = CreateMockTexture(200, "normal.png");
    auto metallicTexture = CreateMockTexture(300, "metallic.png");

    // Set multiple textures on base material
    baseMaterial->SetTexture("u_DiffuseMap", diffuseTexture, 0);
    baseMaterial->SetTexture("u_NormalMap", normalTexture, 1);
    baseMaterial->SetTexture("u_MetallicMap", metallicTexture, 2);

    // Verify all textures
    EXPECT_TRUE(baseMaterial->HasTexture("u_DiffuseMap"));
    EXPECT_TRUE(baseMaterial->HasTexture("u_NormalMap"));
    EXPECT_TRUE(baseMaterial->HasTexture("u_MetallicMap"));
    EXPECT_EQ(baseMaterial->GetTexture("u_DiffuseMap"), diffuseTexture);
    EXPECT_EQ(baseMaterial->GetTexture("u_NormalMap"), normalTexture);
    EXPECT_EQ(baseMaterial->GetTexture("u_MetallicMap"), metallicTexture);
}

TEST_F(MaterialInstanceTest, TextureAutoUnitAssignment) {
    auto texture1 = CreateMockTexture(100, "texture1.png");
    auto texture2 = CreateMockTexture(200, "texture2.png");

    // Set textures with auto-assignment (unit = -1)
    baseMaterial->SetTexture("u_Texture1", texture1, -1);
    baseMaterial->SetTexture("u_Texture2", texture2, -1);

    // Both should be assigned
    EXPECT_TRUE(baseMaterial->HasTexture("u_Texture1"));
    EXPECT_TRUE(baseMaterial->HasTexture("u_Texture2"));
    EXPECT_EQ(baseMaterial->GetTexture("u_Texture1"), texture1);
    EXPECT_EQ(baseMaterial->GetTexture("u_Texture2"), texture2);
}

TEST_F(MaterialInstanceTest, TextureOverrideClear) {
    auto baseTexture = CreateMockTexture(100, "base.png");
    auto overrideTexture = CreateMockTexture(200, "override.png");

    baseMaterial->SetTexture("u_DiffuseMap", baseTexture, 0);

    MaterialInstance instance(baseMaterial);
    instance.SetTexture("u_DiffuseMap", overrideTexture, 1);

    EXPECT_TRUE(instance.IsPropertyOverridden("u_DiffuseMap"));
    EXPECT_EQ(instance.GetTexture("u_DiffuseMap"), overrideTexture);

    // Clear overrides
    instance.ClearOverrides();

    // Should revert to base texture
    EXPECT_FALSE(instance.IsPropertyOverridden("u_DiffuseMap"));
    EXPECT_EQ(instance.GetTexture("u_DiffuseMap"), baseTexture);
}

TEST_F(MaterialInstanceTest, TextureNullHandling) {
    // Getting non-existent texture should return nullptr
    EXPECT_EQ(baseMaterial->GetTexture("u_NonExistent"), nullptr);
    EXPECT_FALSE(baseMaterial->HasTexture("u_NonExistent"));

    MaterialInstance instance(baseMaterial);
    EXPECT_EQ(instance.GetTexture("u_NonExistent"), nullptr);
}

TEST_F(MaterialInstanceTest, TextureReplacement) {
    auto texture1 = CreateMockTexture(100, "texture1.png");
    auto texture2 = CreateMockTexture(200, "texture2.png");

    // Set initial texture
    baseMaterial->SetTexture("u_DiffuseMap", texture1, 0);
    EXPECT_EQ(baseMaterial->GetTexture("u_DiffuseMap"), texture1);

    // Replace with different texture
    baseMaterial->SetTexture("u_DiffuseMap", texture2, 0);
    EXPECT_EQ(baseMaterial->GetTexture("u_DiffuseMap"), texture2);
}

TEST_F(MaterialInstanceTest, TextureAndPropertyMixedOverrides) {
    auto baseTexture = CreateMockTexture(100, "base.png");
    auto overrideTexture = CreateMockTexture(200, "override.png");

    // Set base properties
    baseMaterial->SetTexture("u_DiffuseMap", baseTexture, 0);
    baseMaterial->SetFloat("u_Roughness", 0.5f);
    baseMaterial->SetVec3("u_Albedo", glm::vec3(0.8f, 0.8f, 0.8f));

    MaterialInstance instance(baseMaterial);

    // Override both texture and scalar properties
    instance.SetTexture("u_DiffuseMap", overrideTexture, 1);
    instance.SetFloat("u_Roughness", 0.2f);
    instance.SetVec3("u_Albedo", glm::vec3(1.0f, 0.0f, 0.0f));

    // Verify all overrides
    EXPECT_TRUE(instance.IsPropertyOverridden("u_DiffuseMap"));
    EXPECT_TRUE(instance.IsPropertyOverridden("u_Roughness"));
    EXPECT_TRUE(instance.IsPropertyOverridden("u_Albedo"));

    EXPECT_EQ(instance.GetTexture("u_DiffuseMap"), overrideTexture);
    EXPECT_FLOAT_EQ(instance.GetFloat("u_Roughness", 0.0f), 0.2f);

    glm::vec3 albedo = instance.GetVec3("u_Albedo", glm::vec3(0.0f));
    EXPECT_FLOAT_EQ(albedo.x, 1.0f);
    EXPECT_FLOAT_EQ(albedo.y, 0.0f);
    EXPECT_FLOAT_EQ(albedo.z, 0.0f);

    // Base material should be unchanged
    EXPECT_EQ(baseMaterial->GetTexture("u_DiffuseMap"), baseTexture);
    EXPECT_FLOAT_EQ(baseMaterial->GetFloatProperty("u_Roughness", 0.0f), 0.5f);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(MaterialInstanceManagerTest, UnityStyleWorkflow) {
    uint64_t entity1 = 2001;
    uint64_t entity2 = 2002;
    uint64_t entity3 = 2003;

    // Entity 1: Use shared material (no customization)
    auto sharedMat1 = manager->GetSharedMaterial(baseMaterial);
    EXPECT_EQ(sharedMat1, baseMaterial);
    EXPECT_FALSE(manager->HasInstance(entity1));

    // Entity 2: Create instance and customize
    auto instance2 = manager->GetMaterialInstance(entity2, baseMaterial);
    instance2->SetFloat("u_Roughness", 0.1f); // Smoother
    EXPECT_TRUE(manager->HasInstance(entity2));

    // Entity 3: Create instance with different customization
    auto instance3 = manager->GetMaterialInstance(entity3, baseMaterial);
    instance3->SetVec3("u_Albedo", glm::vec3(1.0f, 0.0f, 0.0f)); // Red
    EXPECT_TRUE(manager->HasInstance(entity3));

    // Verify isolation
    EXPECT_FLOAT_EQ(baseMaterial->GetFloatProperty("u_Roughness", 0.0f), 0.5f); // Unchanged
    EXPECT_FLOAT_EQ(instance2->GetFloat("u_Roughness", 0.0f), 0.1f); // Customized
    EXPECT_FLOAT_EQ(instance3->GetFloat("u_Roughness", 0.0f), 0.5f); // Falls back to base

    // Entity count
    EXPECT_EQ(manager->GetInstanceCount(), 2); // Only entity2 and entity3 have instances
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
