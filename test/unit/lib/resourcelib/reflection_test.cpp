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

TEST(NativeTypes, Vec4Reflection) {
    glm::vec4 v4{ urd_float_random().generate_vec4()};
    const char* v4_fname[4]{"x", "y", "z", "w"};
    auto t = rp::reflection::reflect(v4);
    int i{};
    t.each([&](auto&& v) {
        ASSERT_EQ(v.m_field_name, std::string_view(v4_fname[i]));
        ASSERT_GE(*v.m_field_ptr+FLT_EPSILON, v4[i++]);
        });
}