/******************************************************************************/
/*!
\file   ManagedCamera.cpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the implementation for the ManagedCamera class, which
is responsible for managing camera-related functionalities in the managed environment.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#include "Bindings/ManagedCamera.hpp"

#include "Bindings/ManagedScreen.hpp"
#include "ecs/internal/entity.h"
#include "Render/Camera.h"
#include "Input/InputGraphics_Helper.h"
#include <atomic>

namespace {
	std::atomic<bool> s_ViewportOverrideEnabled{ false };
	std::atomic<float> s_ViewportOffsetX{ 0.0f };
	std::atomic<float> s_ViewportOffsetY{ 0.0f };
	std::atomic<float> s_ViewportWidth{ 0.0f };
	std::atomic<float> s_ViewportHeight{ 0.0f };
}


int ManagedCamera::GetCameraType(uint64_t handle)
{
	ecs::entity target{ handle };
	auto& cameraComponent = target.get<CameraComponent>();
	return static_cast<int>(cameraComponent.m_Type);
}

void ManagedCamera::SetCameraType(uint64_t handle, int type)
{
	ecs::entity target{ handle };
	auto& cameraComponent = target.get<CameraComponent>();
	cameraComponent.m_Type = static_cast<CameraComponent::CameraType>(type);
}

float ManagedCamera::getFoV(uint64_t handle)
{
	ecs::entity target{ handle };
	auto& cameraComponent = target.get<CameraComponent>();
	return cameraComponent.m_Fov;
}

void ManagedCamera::setFoV(uint64_t handle, float fov)
{
	ecs::entity target{ handle };
	auto& cameraComponent = target.get<CameraComponent>();
	cameraComponent.m_Fov = fov;
}

float ManagedCamera::getAspectRatio(uint64_t handle)
{
	ecs::entity target{ handle };
	auto& cameraComponent = target.get<CameraComponent>();
	return cameraComponent.m_AspectRatio;
}

void ManagedCamera::setAspectRatio(uint64_t handle, float aspectRatio)
{
	ecs::entity target{ handle };
	auto& cameraComponent = target.get<CameraComponent>();
	cameraComponent.m_AspectRatio = aspectRatio;
}
float ManagedCamera::getNear(uint64_t handle)
{
	ecs::entity target{ handle };
	auto& cameraComponent = target.get<CameraComponent>();
	return cameraComponent.m_Near;
}
void ManagedCamera::setNear(uint64_t handle, float nearClip)
{
	ecs::entity target{ handle };
	auto& cameraComponent = target.get<CameraComponent>();
	cameraComponent.m_Near = nearClip;
}
float ManagedCamera::getFar(uint64_t handle)
{
	ecs::entity target{ handle };
	auto& cameraComponent = target.get<CameraComponent>();
	return cameraComponent.m_Far;
}
void ManagedCamera::setFar(uint64_t handle, float farClip)
{
	ecs::entity target{ handle };
	auto& cameraComponent = target.get<CameraComponent>();
	cameraComponent.m_Far = farClip;
}

void ManagedCamera::SetViewportOverride(float offsetX, float offsetY, float width, float height, bool enabled)
{
	s_ViewportOverrideEnabled.store(enabled);
	if (enabled) {
		s_ViewportOffsetX.store(offsetX);
		s_ViewportOffsetY.store(offsetY);
		s_ViewportWidth.store(width);
		s_ViewportHeight.store(height);
	}
}

void ManagedCamera::ScreenToWorldPoint(uint64_t handle, float x, float y, float depth, float* p_x, float* p_y, float* p_z)
{
	ecs::entity target{ handle };
	auto& cameraComponent = target.get<CameraComponent>();
	glm::vec2 screenPos = glm::vec2{ x, y };
	(void)cameraComponent; // parameters unused currently; kept for parity with other methods
	glm::vec2 screenSize;
	glm::vec2 offset;
	if (s_ViewportOverrideEnabled.load()) {
		screenSize = glm::vec2{ s_ViewportWidth.load(), s_ViewportHeight.load() };
		offset = glm::vec2{ s_ViewportOffsetX.load(), s_ViewportOffsetY.load() };
	} else {
		screenSize = glm::vec2{ static_cast<float>(ManagedScreen::GetWidth()), static_cast<float>(ManagedScreen::GetHeight()) };
		offset = CameraSystem::GetCachedViewportOffset();
	}
	glm::vec2 localPos = screenPos - offset;
	glm::vec3 world = CameraSystem::GetWorldFromScreen(localPos, screenSize, depth);
	if (p_x) *p_x = world.x;
	if (p_y) *p_y = world.y;
	if (p_z) *p_z = world.z;
}

void ManagedCamera::ScreenPointToRay(uint64_t handle, float x, float y, float /*depth*/,
	float* o_x, float* o_y, float* o_z,
	float* d_x, float* d_y, float* d_z)
{
	ecs::entity target{ handle };
	auto& cameraComponent = target.get<CameraComponent>();
	(void)cameraComponent;
	glm::vec2 screenPos{ x, y };
	glm::vec2 screenSize;
	glm::vec2 offset;
	if (s_ViewportOverrideEnabled.load()) {
		screenSize = glm::vec2{ s_ViewportWidth.load(), s_ViewportHeight.load() };
		offset = glm::vec2{ s_ViewportOffsetX.load(), s_ViewportOffsetY.load() };
	} else {
		screenSize = glm::vec2{ static_cast<float>(ManagedScreen::GetWidth()), static_cast<float>(ManagedScreen::GetHeight()) };
		offset = CameraSystem::GetCachedViewportOffset();
	}
	glm::vec2 localPos = screenPos - offset;
	glm::vec3 origin{}, dir{};
	CameraSystem::GetRayFromScreen(localPos, screenSize, origin, dir);

	if (o_x) *o_x = origin.x;
	if (o_y) *o_y = origin.y;
	if (o_z) *o_z = origin.z;
	if (d_x) *d_x = dir.x;
	if (d_y) *d_y = dir.y;
	if (d_z) *d_z = dir.z;
}
