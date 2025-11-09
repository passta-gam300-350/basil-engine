/******************************************************************************/
/*!
\file   ManagedLight.cpp
\author Team PASSTA
		Hai Jie (haijie.w\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the implementation for the ManagedLight class, which
provides an interface for managing light properties in a managed (C#) environment.


Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Bindings/ManagedLight.hpp"

#include "ecs/internal/entity.h"
#include <glm/vec3.hpp>

#include "Render/Render.h"

static inline LightComponent& get_light(uint64_t h)
{
	ecs::entity e{ h };
	return e.get<LightComponent>();
}

// -------- Type --------
void ManagedLight::SetLightType(uint64_t handle, int type)
{
	auto& lc = get_light(handle);
	lc.m_Type = static_cast<Light::Type>(type);
}

int ManagedLight::GetLightType(uint64_t handle)
{
	auto& lc = get_light(handle);
	return static_cast<int>(lc.m_Type);
}

// -------- Enabled --------
bool ManagedLight::GetEnabled(uint64_t handle)
{
	return get_light(handle).m_IsEnabled;
}

void ManagedLight::SetEnabled(uint64_t handle, bool v)
{
	get_light(handle).m_IsEnabled = v;
}

// -------- Color --------
float ManagedLight::GetColorR(uint64_t handle) { return get_light(handle).m_Color.r; }
float ManagedLight::GetColorG(uint64_t handle) { return get_light(handle).m_Color.g; }
float ManagedLight::GetColorB(uint64_t handle) { return get_light(handle).m_Color.b; }

void ManagedLight::SetColor(uint64_t handle, float r, float g, float b)
{
	get_light(handle).m_Color = { r, g, b };
}

void ManagedLight::SetColorR(uint64_t handle, float r) { get_light(handle).m_Color.r = r; }
void ManagedLight::SetColorG(uint64_t handle, float g) { get_light(handle).m_Color.g = g; }
void ManagedLight::SetColorB(uint64_t handle, float b) { get_light(handle).m_Color.b = b; }

// -------- Intensity --------
float ManagedLight::GetIntensity(uint64_t handle)
{
	return get_light(handle).m_Intensity;
}

void ManagedLight::SetIntensity(uint64_t handle, float v)
{
	get_light(handle).m_Intensity = v;
}

// -------- Direction --------
float ManagedLight::GetDirectionX(uint64_t handle) { return get_light(handle).m_Direction.x; }
float ManagedLight::GetDirectionY(uint64_t handle) { return get_light(handle).m_Direction.y; }
float ManagedLight::GetDirectionZ(uint64_t handle) { return get_light(handle).m_Direction.z; }

void ManagedLight::SetDirection(uint64_t handle, float x, float y, float z)
{
	get_light(handle).m_Direction = { x, y, z };
}

// -------- Range --------
float ManagedLight::GetRange(uint64_t handle)
{
	return get_light(handle).m_Range;
}

void ManagedLight::SetRange(uint64_t handle, float v)
{
	get_light(handle).m_Range = v;
}

// -------- Spot cones (degrees) --------
float ManagedLight::GetInnerConeDeg(uint64_t handle)
{
	return get_light(handle).m_InnerCone; // stored as degrees in LightComponent
}

float ManagedLight::GetOuterConeDeg(uint64_t handle)
{
	return get_light(handle).m_OuterCone; // stored as degrees in LightComponent
}

void ManagedLight::SetCones(uint64_t handle, float innerDeg, float outerDeg)
{
	auto& lc = get_light(handle);
	lc.m_InnerCone = innerDeg;
	lc.m_OuterCone = outerDeg;
}
