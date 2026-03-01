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
#include <native/native.h>
#include <descriptors/descriptors.hpp>

#include "utility.hpp"

TEST(NativeTypes, Vec4YamlSerialization) {
    glm::vec4 v4{ urd_float_random().generate_vec4() };
    rp::serialization::yaml_serializer::serialize(v4, "test.yaml");
    glm::vec4 v4new = rp::serialization::yaml_serializer::deserialize<glm::vec4>("test.yaml");

    int i{};
    auto t = rp::reflection::reflect(v4);
    t.each([&](auto&& v) {
        ASSERT_GE(*v.m_field_ptr + FLT_EPSILON, v4[i++]);
        });
}

TEST(NativeTypes, Vec4BinSerialization) {
    glm::vec4 v4{ urd_float_random().generate_vec4() };
    rp::serialization::binary_serializer::serialize(v4, "test.bin");
    glm::vec4 v4new = rp::serialization::binary_serializer::deserialize<glm::vec4>("test.bin");

    int i{};
    auto t = rp::reflection::reflect(v4);
    t.each([&](auto&& v) {
        ASSERT_GE(*v.m_field_ptr + FLT_EPSILON, v4[i++]);
        });
}