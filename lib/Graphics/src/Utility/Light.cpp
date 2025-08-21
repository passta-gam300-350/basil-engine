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