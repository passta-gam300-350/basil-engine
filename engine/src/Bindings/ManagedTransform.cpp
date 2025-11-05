#include "Bindings/ManagedTransform.hpp"

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
}
void ManagedTransform::SetScale(uint64_t handle, float x, float y, float z)
{
	ecs::entity entity{ handle };
	auto& transform = entity.get<TransformComponent>();
	transform.m_Scale = glm::vec3{x, y, z};
}


