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

#include "ecs/internal/entity.h"
#include "Render/Camera.h"


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

