
#include "Ecs/ecs.h"
#include <glm/glm.hpp>

#include "Component/Transform.hpp"
#include "Render/Render.h"
#include "Render/Camera.h"
TypeInfo ResolveType(TypeName t_name) {
	return entt::resolve(t_name);
}

template<typename T, std::uint32_t r, std::uint32_t c>
float GetMatCellValue(T const& m) {
	return m[c][r];
}

template< typename T, std::uint32_t r, std::uint32_t c>
void SetMatCellValue(T& m, float v) {
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
		InterfaceRegistrationV < &SetMatCellValue<glm::mat2, 0, 0>, &GetMatCellValue<glm::mat2, 0, 0>, "m00" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat2, 0, 1>, &GetMatCellValue<glm::mat2, 0, 1>, "m01" >,

		InterfaceRegistrationV < &SetMatCellValue<glm::mat2, 1, 0>, &GetMatCellValue<glm::mat2, 1, 0>, "m10" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat2, 1, 1>, &GetMatCellValue<glm::mat2, 1, 1>, "m11" >
	);

	RegisterReflectionComponent<glm::mat3>(
		"mat3",
		InterfaceRegistrationV < &SetMatCellValue<glm::mat3,0, 0>, &GetMatCellValue<glm::mat3,0, 0>, "m00" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat3,0, 1>, &GetMatCellValue<glm::mat3,0, 1>, "m01" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat3,0, 2>, &GetMatCellValue<glm::mat3,0, 2>, "m02" >,

		InterfaceRegistrationV < &SetMatCellValue<glm::mat3,1, 0>, &GetMatCellValue<glm::mat3,1, 0>, "m10" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat3,1, 1>, &GetMatCellValue<glm::mat3,1, 1>, "m11" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat3,1, 2>, &GetMatCellValue<glm::mat3,1, 2>, "m12" >,

		InterfaceRegistrationV < &SetMatCellValue<glm::mat3,2, 0>, &GetMatCellValue<glm::mat3,2, 0>, "m20" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat3,2, 1>, &GetMatCellValue<glm::mat3,2, 1>, "m21" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat3,2, 2>, &GetMatCellValue<glm::mat3,2, 2>, "m22" >
	);


	RegisterReflectionComponent<glm::mat4>(
		"mat4",
		InterfaceRegistrationV < &SetMatCellValue<glm::mat4,0, 0>, &GetMatCellValue<glm::mat4,0, 0>, "m00" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat4,0, 1>, &GetMatCellValue<glm::mat4,0, 1>, "m01" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat4,0, 2>, &GetMatCellValue<glm::mat4,0, 2>, "m02" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat4,0, 3>, &GetMatCellValue<glm::mat4,0, 3>, "m03" >,

		InterfaceRegistrationV < &SetMatCellValue<glm::mat4,1, 0>, &GetMatCellValue<glm::mat4,1, 0>, "m10" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat4,1, 1>, &GetMatCellValue<glm::mat4,1, 1>, "m11" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat4,1, 2>, &GetMatCellValue<glm::mat4,1, 2>, "m12" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat4,1, 3>, &GetMatCellValue<glm::mat4,1, 3>, "m13" >,

		InterfaceRegistrationV < &SetMatCellValue<glm::mat4,2, 0>, &GetMatCellValue<glm::mat4,2, 0>, "m20" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat4,2, 1>, &GetMatCellValue<glm::mat4,2, 1>, "m21" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat4,2, 2>, &GetMatCellValue<glm::mat4,2, 2>, "m22" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat4,2, 3>, &GetMatCellValue<glm::mat4,2, 3>, "m23" >,

		InterfaceRegistrationV < &SetMatCellValue<glm::mat4, 3, 0>, &GetMatCellValue<glm::mat4,3, 0>, "m30" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat4,3, 1>, &GetMatCellValue<glm::mat4,3, 1>, "m31" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat4, 3, 2>, &GetMatCellValue<glm::mat4,3, 2>, "m32" >,
		InterfaceRegistrationV < &SetMatCellValue<glm::mat4, 3, 3>, &GetMatCellValue<glm::mat4,3, 3>, "m33" >
	);


	

}

void ReflectionRegistry::SetupEngineTypes()
{

	RegisterReflectionComponent<MeshRendererComponent::Material>(
		"Material",
		MemberRegistrationV<&MeshRendererComponent::Material::m_MaterialGuid, "m_MaterialGuid">,
		MemberRegistrationV<&MeshRendererComponent::Material::m_AlbedoColor, "m_AlbedoColor">
	);

	RegisterReflectionComponent<VisibilityComponent>(
		"VisibilityComponent",
		MemberRegistrationV<&VisibilityComponent::m_IsVisible, "m_IsVisible">
	);

	RegisterReflectionComponent<TransformComponent>(
		"TransformComponent",
		MemberRegistrationV<&TransformComponent::m_Scale, "m_Scale">,
		MemberRegistrationV<&TransformComponent::m_Rotation, "m_Rotate">,
		MemberRegistrationV<&TransformComponent::m_Translation, "m_Trans">
	);

	RegisterReflectionComponent<MeshRendererComponent>(
		"MeshRendererComponent",
		MemberRegistrationV<&MeshRendererComponent::m_PrimitiveType, "m_PrimitiveType">,
		MemberRegistrationV<&MeshRendererComponent::hasAttachedMaterial, "hasAttachedMaterial">,
		MemberRegistrationV<&MeshRendererComponent::isPrimitive, "isPrimitive">,
		MemberRegistrationV<&MeshRendererComponent::m_MeshGuid, "m_MeshGuid">,
		MemberRegistrationV<&MeshRendererComponent::m_MaterialGuid, "m_MaterialGuid">,
		MemberRegistrationV<&MeshRendererComponent::material, "material">

	);

	//RegisterReflectionComponent<RigidBodyComponent>(
	//			"RigidBodyComponent",
	//	MemberRegistrationV<&RigidBodyComponent::bodyID, "bodyID">,
	//	MemberRegistrationV<&RigidBodyComponent::motionType, "motionType">,
	//	MemberRegistrationV<&RigidBodyComponent::velocity, "velocity">,
	//	MemberRegistrationV<&RigidBodyComponent::angularVelocity, "angularVelocity">,
	//	MemberRegistrationV<&RigidBodyComponent::mass, "mass">,
	//	MemberRegistrationV<&RigidBodyComponent::isActive, "isActive">
	//);

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
