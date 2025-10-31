/******************************************************************************/
/*!
\file   material_property_block_test.cpp
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/22
\brief  Unit tests for MaterialPropertyBlock system

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#include <gtest/gtest.h>
#include <Resources/MaterialPropertyBlock.h>
#include <Resources/Shader.h>
#include <Resources/Texture.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

// ============================================================================
// MaterialPropertyBlock - Basic Property Storage Tests
// ============================================================================

TEST(MaterialPropertyBlock, DefaultConstructionCreatesEmptyBlock) {
    MaterialPropertyBlock propBlock;

    EXPECT_TRUE(propBlock.IsEmpty());
    EXPECT_EQ(propBlock.GetPropertyCount(), 0);
}

TEST(MaterialPropertyBlock, SetFloatStoresFloatProperty) {
    MaterialPropertyBlock propBlock;

    propBlock.SetFloat("u_Roughness", 0.8f);

    EXPECT_FALSE(propBlock.IsEmpty());
    EXPECT_EQ(propBlock.GetPropertyCount(), 1);
    EXPECT_TRUE(propBlock.HasProperty("u_Roughness"));

    float value;
    EXPECT_TRUE(propBlock.TryGetFloat("u_Roughness", value));
    EXPECT_NEAR(value, 0.8f, 0.001f);
}

TEST(MaterialPropertyBlock, SetVec3StoresVec3Property) {
    MaterialPropertyBlock propBlock;

    glm::vec3 red(1.0f, 0.0f, 0.0f);
    propBlock.SetVec3("u_AlbedoColor", red);

    EXPECT_FALSE(propBlock.IsEmpty());
    EXPECT_TRUE(propBlock.HasProperty("u_AlbedoColor"));

    glm::vec3 value;
    EXPECT_TRUE(propBlock.TryGetVec3("u_AlbedoColor", value));
    EXPECT_FLOAT_EQ(value.x, 1.0f);
    EXPECT_FLOAT_EQ(value.y, 0.0f);
    EXPECT_FLOAT_EQ(value.z, 0.0f);
}

TEST(MaterialPropertyBlock, SetVec4StoresVec4Property) {
    MaterialPropertyBlock propBlock;

    glm::vec4 color(0.5f, 0.75f, 1.0f, 0.8f);
    propBlock.SetVec4("u_Color", color);

    EXPECT_TRUE(propBlock.HasProperty("u_Color"));

    glm::vec4 value;
    EXPECT_TRUE(propBlock.TryGetVec4("u_Color", value));
    EXPECT_NEAR(value.x, 0.5f, 0.001f);
    EXPECT_NEAR(value.y, 0.75f, 0.001f);
    EXPECT_NEAR(value.z, 1.0f, 0.001f);
    EXPECT_NEAR(value.w, 0.8f, 0.001f);
}

TEST(MaterialPropertyBlock, SetMat4StoresMat4Property) {
    MaterialPropertyBlock propBlock;

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));
    propBlock.SetMat4("u_Transform", transform);

    EXPECT_TRUE(propBlock.HasProperty("u_Transform"));

    glm::mat4 value;
    EXPECT_TRUE(propBlock.TryGetMat4("u_Transform", value));
    EXPECT_EQ(value, transform);
}

// ============================================================================
// MaterialPropertyBlock - Texture Property Tests
// ============================================================================

TEST(MaterialPropertyBlock, SetTextureStoresTextureWithUnit) {
    MaterialPropertyBlock propBlock;

    auto texture = std::make_shared<Texture>();
    texture->id = 42;
    texture->type = "diffuse";

    propBlock.SetTexture("u_MainTex", texture, 0);

    EXPECT_TRUE(propBlock.HasProperty("u_MainTex"));

    std::shared_ptr<Texture> retrievedTexture;
    int unit;
    EXPECT_TRUE(propBlock.TryGetTexture("u_MainTex", retrievedTexture, unit));
    EXPECT_EQ(retrievedTexture->id, 42);
    EXPECT_EQ(unit, 0);
}

TEST(MaterialPropertyBlock, SetTextureAutoAssignmentUsesSequentialUnits) {
    MaterialPropertyBlock propBlock;

    auto tex1 = std::make_shared<Texture>();
    auto tex2 = std::make_shared<Texture>();
    auto tex3 = std::make_shared<Texture>();

    propBlock.SetTexture("u_Tex1", tex1); // Auto-assign (unit 0)
    propBlock.SetTexture("u_Tex2", tex2); // Auto-assign (unit 1)
    propBlock.SetTexture("u_Tex3", tex3); // Auto-assign (unit 2)

    std::shared_ptr<Texture> retrievedTex;
    int unit;

    EXPECT_TRUE(propBlock.TryGetTexture("u_Tex1", retrievedTex, unit));
    EXPECT_EQ(unit, 0);

    EXPECT_TRUE(propBlock.TryGetTexture("u_Tex2", retrievedTex, unit));
    EXPECT_EQ(unit, 1);

    EXPECT_TRUE(propBlock.TryGetTexture("u_Tex3", retrievedTex, unit));
    EXPECT_EQ(unit, 2);
}

// ============================================================================
// MaterialPropertyBlock - Multiple Properties Tests
// ============================================================================

TEST(MaterialPropertyBlock, CanStoreMultiplePropertiesOfDifferentTypes) {
    MaterialPropertyBlock propBlock;

    propBlock.SetFloat("u_Metallic", 0.7f);
    propBlock.SetFloat("u_Roughness", 0.3f);
    propBlock.SetVec3("u_AlbedoColor", glm::vec3(0.8f, 0.2f, 0.1f));
    propBlock.SetVec4("u_EmissionColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    EXPECT_EQ(propBlock.GetPropertyCount(), 4);
    EXPECT_FALSE(propBlock.IsEmpty());

    EXPECT_TRUE(propBlock.HasProperty("u_Metallic"));
    EXPECT_TRUE(propBlock.HasProperty("u_Roughness"));
    EXPECT_TRUE(propBlock.HasProperty("u_AlbedoColor"));
    EXPECT_TRUE(propBlock.HasProperty("u_EmissionColor"));
}

// ============================================================================
// MaterialPropertyBlock - Property Overwrite Tests
// ============================================================================

TEST(MaterialPropertyBlock, OverwritingPropertyUpdatesValue) {
    MaterialPropertyBlock propBlock;

    propBlock.SetFloat("u_Roughness", 0.5f);
    propBlock.SetFloat("u_Roughness", 0.9f); // Overwrite

    EXPECT_EQ(propBlock.GetPropertyCount(), 1); // Still only 1 property

    float value;
    EXPECT_TRUE(propBlock.TryGetFloat("u_Roughness", value));
    EXPECT_NEAR(value, 0.9f, 0.001f);
}

TEST(MaterialPropertyBlock, CanChangePropertyType) {
    MaterialPropertyBlock propBlock;

    propBlock.SetFloat("u_Value", 1.0f);
    propBlock.SetVec3("u_Value", glm::vec3(1.0f, 2.0f, 3.0f)); // Change type

    EXPECT_EQ(propBlock.GetPropertyCount(), 1);

    // Should now be vec3, not float
    float floatValue;
    EXPECT_FALSE(propBlock.TryGetFloat("u_Value", floatValue));

    glm::vec3 vec3Value;
    EXPECT_TRUE(propBlock.TryGetVec3("u_Value", vec3Value));
}

// ============================================================================
// MaterialPropertyBlock - Clear and Reset Tests
// ============================================================================

TEST(MaterialPropertyBlock, ClearRemovesAllProperties) {
    MaterialPropertyBlock propBlock;

    propBlock.SetFloat("u_Metallic", 0.7f);
    propBlock.SetVec3("u_AlbedoColor", glm::vec3(1.0f, 0.0f, 0.0f));

    EXPECT_FALSE(propBlock.IsEmpty());
    EXPECT_EQ(propBlock.GetPropertyCount(), 2);

    propBlock.Clear();

    EXPECT_TRUE(propBlock.IsEmpty());
    EXPECT_EQ(propBlock.GetPropertyCount(), 0);
    EXPECT_FALSE(propBlock.HasProperty("u_Metallic"));
    EXPECT_FALSE(propBlock.HasProperty("u_AlbedoColor"));
}

// ============================================================================
// MaterialPropertyBlock - CopyFrom Tests
// ============================================================================

TEST(MaterialPropertyBlock, CopyFromCopiesAllProperties) {
    MaterialPropertyBlock source;
    source.SetFloat("u_Roughness", 0.8f);
    source.SetVec3("u_Color", glm::vec3(1.0f, 0.0f, 0.0f));

    MaterialPropertyBlock dest;
    dest.CopyFrom(source);

    EXPECT_EQ(dest.GetPropertyCount(), 2);
    EXPECT_TRUE(dest.HasProperty("u_Roughness"));
    EXPECT_TRUE(dest.HasProperty("u_Color"));

    float roughness;
    EXPECT_TRUE(dest.TryGetFloat("u_Roughness", roughness));
    EXPECT_NEAR(roughness, 0.8f, 0.001f);

    glm::vec3 color;
    EXPECT_TRUE(dest.TryGetVec3("u_Color", color));
    EXPECT_EQ(color, glm::vec3(1.0f, 0.0f, 0.0f));
}

TEST(MaterialPropertyBlock, CopyFromReplacesExistingProperties) {
    MaterialPropertyBlock source;
    source.SetFloat("u_NewProp", 123.0f);

    MaterialPropertyBlock dest;
    dest.SetFloat("u_OldProp", 456.0f);

    dest.CopyFrom(source);

    EXPECT_EQ(dest.GetPropertyCount(), 1);
    EXPECT_TRUE(dest.HasProperty("u_NewProp"));
    EXPECT_FALSE(dest.HasProperty("u_OldProp"));
}

// ============================================================================
// MaterialPropertyBlock - Query Non-Existent Properties
// ============================================================================

TEST(MaterialPropertyBlock, TryGetReturnsFalseForNonExistentProperty) {
    MaterialPropertyBlock propBlock;

    float floatValue;
    glm::vec3 vec3Value;
    glm::vec4 vec4Value;
    glm::mat4 mat4Value;

    EXPECT_FALSE(propBlock.TryGetFloat("u_NonExistent", floatValue));
    EXPECT_FALSE(propBlock.TryGetVec3("u_NonExistent", vec3Value));
    EXPECT_FALSE(propBlock.TryGetVec4("u_NonExistent", vec4Value));
    EXPECT_FALSE(propBlock.TryGetMat4("u_NonExistent", mat4Value));
}

TEST(MaterialPropertyBlock, TryGetDoesNotModifyOutputOnFailure) {
    MaterialPropertyBlock propBlock;

    float originalValue = 999.0f;
    float testValue = originalValue;

    bool result = propBlock.TryGetFloat("u_NonExistent", testValue);

    EXPECT_FALSE(result);
    EXPECT_EQ(testValue, originalValue); // Value unchanged
}

// ============================================================================
// MaterialPropertyBlock - Type Safety Tests
// ============================================================================

TEST(MaterialPropertyBlock, TryGetFloatReturnsFalseForVec3Property) {
    MaterialPropertyBlock propBlock;

    propBlock.SetVec3("u_Color", glm::vec3(1.0f, 0.0f, 0.0f));

    float value;
    EXPECT_FALSE(propBlock.TryGetFloat("u_Color", value)); // Wrong type
}

TEST(MaterialPropertyBlock, TryGetVec3ReturnsFalseForFloatProperty) {
    MaterialPropertyBlock propBlock;

    propBlock.SetFloat("u_Roughness", 0.5f);

    glm::vec3 value;
    EXPECT_FALSE(propBlock.TryGetVec3("u_Roughness", value)); // Wrong type
}

// ============================================================================
// MaterialPropertyBlock - Use Case Scenario Tests
// ============================================================================

TEST(MaterialPropertyBlockScenarios, PerObjectColorTint) {
    // Scenario: Apply red tint to specific grass blades in a field
    MaterialPropertyBlock grassPropBlock;

    grassPropBlock.SetVec3("u_AlbedoColor", glm::vec3(1.2f, 0.8f, 0.8f)); // Red tint
    grassPropBlock.SetFloat("u_WindStrength", 0.5f);                       // Custom wind

    EXPECT_EQ(grassPropBlock.GetPropertyCount(), 2);
    EXPECT_FALSE(grassPropBlock.IsEmpty());

    // Verify properties can be retrieved for rendering
    glm::vec3 tint;
    float windStrength;
    EXPECT_TRUE(grassPropBlock.TryGetVec3("u_AlbedoColor", tint));
    EXPECT_TRUE(grassPropBlock.TryGetFloat("u_WindStrength", windStrength));
}

TEST(MaterialPropertyBlockScenarios, DamageEffectOverlay) {
    // Scenario: Highlight damaged entities with red overlay
    MaterialPropertyBlock damagePropBlock;

    damagePropBlock.SetVec3("u_EmissionColor", glm::vec3(1.0f, 0.0f, 0.0f)); // Red glow
    damagePropBlock.SetFloat("u_EmissionStrength", 0.8f);                     // Strong emission
    damagePropBlock.SetFloat("u_DamageFlashTime", 0.5f);                      // Flash duration

    EXPECT_EQ(damagePropBlock.GetPropertyCount(), 3);
}

TEST(MaterialPropertyBlockScenarios, LODBasedMaterialTweaks) {
    // Scenario: Reduce quality for distant objects
    MaterialPropertyBlock lodPropBlock;

    // Distant objects get rougher shading (less specular detail)
    lodPropBlock.SetFloat("u_Roughness", 0.9f);
    // Reduce metallic detail
    lodPropBlock.SetFloat("u_MetallicValue", 0.0f);

    EXPECT_EQ(lodPropBlock.GetPropertyCount(), 2);
}

// ============================================================================
// MaterialPropertyBlock - Performance Characteristics
// ============================================================================

TEST(MaterialPropertyBlockPerformance, EmptyBlockHasZeroOverhead) {
    MaterialPropertyBlock emptyBlock;

    EXPECT_TRUE(emptyBlock.IsEmpty());
    EXPECT_EQ(emptyBlock.GetPropertyCount(), 0);
}

TEST(MaterialPropertyBlockPerformance, CanHandleManyProperties) {
    MaterialPropertyBlock propBlock;

    // Add 100 properties
    for (int i = 0; i < 100; ++i) {
        propBlock.SetFloat("u_Prop" + std::to_string(i), static_cast<float>(i));
    }

    EXPECT_EQ(propBlock.GetPropertyCount(), 100);

    // Verify retrieval works
    float value;
    EXPECT_TRUE(propBlock.TryGetFloat("u_Prop42", value));
    EXPECT_NEAR(value, 42.0f, 0.001f);
}
