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

