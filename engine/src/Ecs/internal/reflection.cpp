
#include "Ecs/ecs.h"
#include <glm/glm.hpp>

#include "components/transform.h"
#include "Render/Render.h"
TypeInfo ResolveType(TypeName t_name) {
	return entt::resolve(t_name);
}

template<std::uint32_t r, std::uint32_t c>
float GetMat4CellValue(glm::mat4 const& m) {
	return m[c][r];
}

template<std::uint32_t r, std::uint32_t c>
void SetMat4CellValue(glm::mat4& m, float v) {
	m[c][r] = v;
}
	






void ReflectionRegistry::SetupNativeTypes() {

	

	RegisterReflectionComponent<ecs::entity::entity_name_t>(
		"entity name",
		MemberRegistrationV<&ecs::entity::entity_name_t::m_name, "name">
	);

	


	RegisterReflectionComponent<glm::vec2>(
		"vec2",
		MemberRegistrationV<&glm::vec2::x, "x">,
		MemberRegistrationV<&glm::vec2::y, "y">
	);

	RegisterReflectionComponent<glm::vec3>(
		"vec3",
		MemberRegistrationV<&glm::vec3::x, "x">,
		MemberRegistrationV<&glm::vec3::y, "y">,
		MemberRegistrationV<&glm::vec3::z, "z">
	);

	RegisterReflectionComponent<glm::vec4>(
		"vec4",
		MemberRegistrationV<&glm::vec4::x, "x">,
		MemberRegistrationV<&glm::vec4::y, "y">,
		MemberRegistrationV<&glm::vec4::z, "z">,
		MemberRegistrationV<&glm::vec4::w, "w">
	);

	RegisterReflectionComponent<glm::mat2>(
		"mat2",
		MemberRegistrationV < [](glm::mat2 mtx) -> glm::vec2& { return mtx[0]; }, "m0" > ,
		MemberRegistrationV < [](glm::mat2 mtx) -> glm::vec2& { return mtx[1]; }, "m1" >
	);

	RegisterReflectionComponent<glm::mat3>(
		"mat3",
		MemberRegistrationV < [](glm::mat3 mtx) -> glm::vec3& { return mtx[0]; }, "m0" > ,
		MemberRegistrationV < [](glm::mat3 mtx) -> glm::vec3& { return mtx[1]; }, "m1" > ,
		MemberRegistrationV < [](glm::mat3 mtx) -> glm::vec3& { return mtx[2]; }, "m2" >
	);
	/*
	RegisterReflectionComponent<glm::mat4>(
		"mat4",
		MemberRegistrationV<[](glm::mat4 mtx) -> glm::vec4& { return mtx[0]; }, "m0">,
		MemberRegistrationV<[](glm::mat4 mtx) -> glm::vec4& { return mtx[1]; }, "m1">,
		MemberRegistrationV<[](glm::mat4 mtx) -> glm::vec4& { return mtx[2]; }, "m2">,
		MemberRegistrationV<[](glm::mat4 mtx) -> glm::vec4& { return mtx[3]; }, "m3">
	);
	*/
	RegisterReflectionComponent<glm::mat4>(
		"mat4",
		InterfaceRegistrationV < &SetMat4CellValue<0, 0>, &GetMat4CellValue<0, 0>, "m00" >,
		InterfaceRegistrationV < &SetMat4CellValue<0, 1>, &GetMat4CellValue<0, 1>, "m01" >,
		InterfaceRegistrationV < &SetMat4CellValue<0, 2>, &GetMat4CellValue<0, 2>, "m02" >,
		InterfaceRegistrationV < &SetMat4CellValue<0, 3>, &GetMat4CellValue<0, 3>, "m03" >,

		InterfaceRegistrationV < &SetMat4CellValue<1, 0>, &GetMat4CellValue<1, 0>, "m10" >,
		InterfaceRegistrationV < &SetMat4CellValue<1, 1>, &GetMat4CellValue<1, 1>, "m11" >,
		InterfaceRegistrationV < &SetMat4CellValue<1, 2>, &GetMat4CellValue<1, 2>, "m12" >,
		InterfaceRegistrationV < &SetMat4CellValue<1, 3>, &GetMat4CellValue<1, 3>, "m13" >,

		InterfaceRegistrationV < &SetMat4CellValue<2, 0>, &GetMat4CellValue<2, 0>, "m20" >,
		InterfaceRegistrationV < &SetMat4CellValue<2, 1>, &GetMat4CellValue<2, 1>, "m21" >,
		InterfaceRegistrationV < &SetMat4CellValue<2, 2>, &GetMat4CellValue<2, 2>, "m22" >,
		InterfaceRegistrationV < &SetMat4CellValue<2, 3>, &GetMat4CellValue<2, 3>, "m23" >,

		InterfaceRegistrationV < &SetMat4CellValue<3, 0>, &GetMat4CellValue<3, 0>, "m30" >,
		InterfaceRegistrationV < &SetMat4CellValue<3, 1>, &GetMat4CellValue<3, 1>, "m31" >,
		InterfaceRegistrationV < &SetMat4CellValue<3, 2>, &GetMat4CellValue<3, 2>, "m32" >,
		InterfaceRegistrationV < &SetMat4CellValue<3, 3>, &GetMat4CellValue<3, 3>, "m33" >
	);


	

}

void ReflectionRegistry::SetupEngineTypes()
{

	RegisterReflectionComponent<VisibilityComponent>(
		"VisibilityComponent",
		MemberRegistrationV<&VisibilityComponent::m_IsVisible, "m_IsVisible">
	);

	RegisterReflectionComponent<PositionComponent>(
		"PositionComponent",
		MemberRegistrationV<&PositionComponent::m_WorldPos, "m_WorldPos">
	);

	RegisterReflectionComponent<ScaleComponent>(
		"ScaleComponent",
		MemberRegistrationV<&ScaleComponent::m_Scale, "m_Scale">
	);

	RegisterReflectionComponent<RotationComponent>(
		"RotationComponent",
		MemberRegistrationV<&RotationComponent::m_Rotation, "m_Rotation">

	);

	RegisterReflectionComponent<TransformComponent>(
		"TransformComponent",
		MemberRegistrationV<&TransformComponent::m_trans, "m_trans">
	);

	RegisterReflectionComponent<MeshRendererComponent>(
		"MeshRendererComponent",
		MemberRegistrationV<&MeshRendererComponent::m_PrimitiveType, "m_PrimitiveType">,
		MemberRegistrationV<&MeshRendererComponent::isPrimitive, "isPrimitive">,
		MemberRegistrationV<&MeshRendererComponent::m_MeshGuid, "m_MeshGuid">,
		MemberRegistrationV<&MeshRendererComponent::m_MaterialGuid, "m_MaterialGuid">
	);



	RegisterReflectionComponent<LightComponent>(
		"LightComponent",
		MemberRegistrationV<&LightComponent::m_Type, "m_Type">,
		MemberRegistrationV<&LightComponent::m_Direction, "m_Direction">,
		MemberRegistrationV<&LightComponent::m_Color, "m_Color">,
		MemberRegistrationV<&LightComponent::m_Intensity, "m_Intensity">,
		MemberRegistrationV<&LightComponent::m_Range, "m_Range">,
		MemberRegistrationV<&LightComponent::m_InnerCone, "m_InnerCone">,
		MemberRegistrationV<&LightComponent::m_OuterCone, "m_OuterCone">,
		MemberRegistrationV<&LightComponent::m_IsEnabled, "m_IsEnabled">
	);

	RegisterReflectionComponent<CameraComponent>(
		"CameraComponent",
		MemberRegistrationV<&CameraComponent::m_Type, "m_Type">,
		MemberRegistrationV<&CameraComponent::m_IsActive, "m_IsActive">,
		MemberRegistrationV<&CameraComponent::m_Fov, "m_Fov">,
		MemberRegistrationV<&CameraComponent::m_AspectRatio, "m_AspectRatio">,
		MemberRegistrationV<&CameraComponent::m_Near, "m_Near">,
		MemberRegistrationV<&CameraComponent::m_Far, "m_Far">,
		MemberRegistrationV<&CameraComponent::m_Up, "m_Up">,
		MemberRegistrationV<&CameraComponent::m_Right, "m_Right">,
		MemberRegistrationV<&CameraComponent::m_Front, "m_Front">,
		MemberRegistrationV<&CameraComponent::m_Yaw, "m_Yaw">,
		MemberRegistrationV<&CameraComponent::m_Pitch, "m_Pitch">
	);
}
