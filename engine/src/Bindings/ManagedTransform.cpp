/******************************************************************************/
/*!
\file   ManagedTransform.cpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the implementation for the ManagedTransform class, which
is responsible for managing and getting various transform-related functionalities in
the managed environment.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#include "Bindings/ManagedTransform.hpp"
#include "Component/Transform.hpp"

namespace
{
	void ExtractBasis(uint64_t handle, glm::vec3& outRight, glm::vec3& outUp, glm::vec3& outForward)
	{
		ecs::entity entity{ handle };

		if (entity.all<TransformMtxComponent>())
		{
			const auto& mtx = entity.get<TransformMtxComponent>().m_Mtx;
			outRight = glm::vec3(mtx[0]);   // col0
			outUp = glm::vec3(mtx[1]);      // col1
			outForward = glm::vec3(mtx[2]); // col2
		}
		else
		{
			const auto& transform = entity.get<TransformComponent>();
			auto quat = glm::quat(glm::radians(transform.m_Rotation));
			outRight = quat * glm::vec3(1.f, 0.f, 0.f);
			outUp = quat * glm::vec3(0.f, 1.f, 0.f);
			outForward = quat * glm::vec3(0.f, 0.f, 1.f);
		}

		outRight = glm::normalize(outRight);
		outUp = glm::normalize(outUp);
		outForward = glm::normalize(outForward);
	}
}

void ManagedTransform::GetPosition(uint64_t handle, float* x, float* y, float* z)
{
	ecs::entity entity{ handle };
	auto& transform = entity.get<TransformComponent>();
	*x = transform.m_Translation.x;
	*y = transform.m_Translation.y;
	*z = transform.m_Translation.z;

}
void ManagedTransform::GetRotation(uint64_t handle, float* pitch, float* yaw, float* roll)
{
	ecs::entity entity{ handle };
	auto& transform = entity.get<TransformComponent>();
	*pitch = transform.m_Rotation.x;
	*yaw = transform.m_Rotation.y;
	*roll = transform.m_Rotation.z;
}

void ManagedTransform::GetScale(uint64_t handle, float* x, float* y, float* z)
{
	ecs::entity entity{ handle };
	auto& transform = entity.get<TransformComponent>();
	*x = transform.m_Scale.x;
	*y = transform.m_Scale.y;
	*z = transform.m_Scale.z;
}

void ManagedTransform::SetPosition(uint64_t handle, float x, float y, float z)
{
	ecs::entity entity{ handle };
	auto& transform = entity.get<TransformComponent>();
	transform.m_Translation = glm::vec3{x, y, z};
}
void ManagedTransform::SetRotation(uint64_t handle, float pitch, float yaw, float roll)
{
	ecs::entity entity{ handle };
	auto& transform = entity.get<TransformComponent>();
	transform.m_Rotation = glm::vec3{pitch, yaw, roll};
	std::cout << "Set Rotation to: " << pitch << ", " << yaw << ", " << roll << std::endl;
}
void ManagedTransform::SetScale(uint64_t handle, float x, float y, float z)
{
	ecs::entity entity{ handle };
	auto& transform = entity.get<TransformComponent>();
	transform.m_Scale = glm::vec3{x, y, z};
}


void ManagedTransform::GetForward(uint64_t handle, float* x, float* y, float* z)
{
	glm::vec3 right{}, up{}, forward{};
	ExtractBasis(handle, right, up, forward);
	*x = forward.x;
	*y = forward.y;
	*z = forward.z;
}

void ManagedTransform::GetRight(uint64_t handle, float* x, float* y, float* z)
{
	glm::vec3 right{}, up{}, forward{};
	ExtractBasis(handle, right, up, forward);
	*x = right.x;
	*y = right.y;
	*z = right.z;
}

void ManagedTransform::GetUp(uint64_t handle, float* x, float* y, float* z)
{
	glm::vec3 right{}, up{}, forward{};
	ExtractBasis(handle, right, up, forward);
	*x = up.x;
	*y = up.y;
	*z = up.z;
}


