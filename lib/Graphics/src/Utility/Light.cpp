/******************************************************************************/
/*!
\file   Light.cpp
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Implementation of light properties for directional, point, and spot lights

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "../../include/Utility/Light.h"
#include <glm/gtc/matrix_transform.hpp>

Light::Light(Type type)
	: m_Type(type)
{
}

void Light::SetAttenuation(float constant, float linear, float quadratic)
{
    m_Attenuation = { constant, linear, quadratic };
}

void Light::SetCutoff(float innerCutoff, float outerCutoff)
{
    m_InnerCutoff = glm::cos(glm::radians(innerCutoff));
    m_OuterCutoff = glm::cos(glm::radians(outerCutoff));
}