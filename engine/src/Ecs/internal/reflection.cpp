/******************************************************************************/
/*!
\file   reflection.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  ECS reflection implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Ecs/ecs.h"
#include <glm/glm.hpp>

#include "Component/Transform.hpp"
#include "Component/MaterialOverridesComponent.hpp"
#include "Render/Render.h"
#include "Particles/ParticleComponent.h"

#include "Render/Camera.h"
#include "Utility/Particle.h"

#include <components/behaviour.hpp>

#include <Jolt/Jolt.h>

#include "Input/Button.h"
#include "System/Audio.hpp"
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

	RegisterReflectionComponent<JPH::Vec3>(
		"JPHvec3",
		InterfaceRegistrationV < &JPH::Vec3::SetX , &JPH::Vec3::GetX, "x" >,
		InterfaceRegistrationV < &JPH::Vec3::SetY, &JPH::Vec3::GetY, "y" >,
		InterfaceRegistrationV < &JPH::Vec3::SetZ, &JPH::Vec3::GetZ, "z" >
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

	// Register vector of strings
	
	/*RegisterReflectionContainer<std::vector<std::string>, std::string>(
		"vector<string>",
		nullptr
	);*/

	/*RegisterReflectionContainer<std::vector<std::string>>(
		"vector<string>",
		nullptr
	);*/

	

}

void ReflectionRegistry::SetupEngineTypes()
{

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

	/*RegisterReflectionComponent<rp::BasicIndexedGuid>(
		"IndexedGuid",
		MemberRegistrationV<&rp::BasicIndexedGuid::m_guid, "Guid">,
		MemberRegistrationV<&rp::BasicIndexedGuid::m_typeindex, "Type Index">
	);*/

	RegisterReflectionComponent<MeshRendererComponent::PrimitiveType>("PrimitiveType");

	RegisterReflectionComponent<MeshRendererComponent>(
		"MeshRendererComponent",
		MemberRegistrationV<&MeshRendererComponent::m_PrimitiveType, "m_PrimitiveType">,
		MemberRegistrationV<&MeshRendererComponent::hasAttachedMaterial, "hasAttachedMaterial">,
		MemberRegistrationV<&MeshRendererComponent::isPrimitive, "isPrimitive">,
		MemberRegistrationV<&MeshRendererComponent::m_MeshGuid, "m_MeshGuid">,
		MemberRegistrationV<&MeshRendererComponent::m_MaterialGuid, "m_MaterialGuid">
	);

	RegisterReflectionComponent<Light::Type>("Light Type");

	RegisterReflectionComponent<LightComponent>(
		"LightComponent",
		MemberRegistrationV<&LightComponent::m_Type, "Type">,
		MemberRegistrationV<&LightComponent::m_Color, "Color">,
		MemberRegistrationV<&LightComponent::m_IsEnabled, "Enabled">,
		MemberRegistrationV<&LightComponent::m_CastShadows, "Cast Shadows">,
		MemberRegistrationV<&LightComponent::m_Intensity, "Intensity">,
		MemberRegistrationV<&LightComponent::m_Direction, "[Dir/Spot] Direction">,
		MemberRegistrationV<&LightComponent::m_Range, "[Point/Spot] Range">,
		MemberRegistrationV<&LightComponent::m_InnerCone, "[Spot] Inner Cone Angle">,
		MemberRegistrationV<&LightComponent::m_OuterCone, "[Spot] Outer Cone Angle">
	);

	// Register CameraComponent enum type
	RegisterReflectionComponent<CameraComponent::CameraType>("CameraComponent::CameraType");

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

	RegisterReflectionComponent<MaterialOverridesComponent>(
		"MaterialOverridesComponent",
		MemberRegistrationV<&MaterialOverridesComponent::floatOverrides, "floatOverrides">,
		MemberRegistrationV<&MaterialOverridesComponent::vec3Overrides, "vec3Overrides">,
		MemberRegistrationV<&MaterialOverridesComponent::vec4Overrides, "vec4Overrides">,
		MemberRegistrationV<&MaterialOverridesComponent::mat4Overrides, "mat4Overrides">
	);

	// Register particle enums
	RegisterReflectionComponent<EmissionType>("EmissionType");
	RegisterReflectionComponent<BlendMode>("BlendMode");


	RegisterReflectionComponent<behaviour>(
		"Behaviour",
		MemberRegistrationV<&behaviour::classesName, "classesName">,
		MemberRegistrationV<&behaviour::scriptProperties, "scriptProperties">
	);

	RegisterReflectionComponent<ScriptProperty>(
		"ScriptProperty",
		MemberRegistrationV<&ScriptProperty::name, "name">,
		MemberRegistrationV<&ScriptProperty::typeName, "type">,
		MemberRegistrationV<&ScriptProperty::value, "value">,
		MemberRegistrationV<&ScriptProperty::is_user_type, "is_user_type">
	);

	// Audio groups
	//RegisterReflectionComponent<AudioGroup>("AudioGroup");

	RegisterReflectionComponent<AudioComponent>(
		"AudioComponent",
		MemberRegistrationV<&AudioComponent::audioAssetGuid, "audioAssetGuid">,
		//audio group
		//filter array
		MemberRegistrationV<&AudioComponent::group, "group">,
		MemberRegistrationV<&AudioComponent::volume, "volume">,
		MemberRegistrationV<&AudioComponent::isLooping, "isLooping">,
		MemberRegistrationV<&AudioComponent::is3D, "is3D">,
		MemberRegistrationV<&AudioComponent::isStreaming, "isStreaming">,
		MemberRegistrationV<&AudioComponent::playOnAwake, "playOnAwake">,
		MemberRegistrationV<&AudioComponent::minDistance, "minDistance">,
		MemberRegistrationV<&AudioComponent::maxDistance, "maxDistance">,
		MemberRegistrationV<&AudioComponent::position, "position">,
		MemberRegistrationV<&AudioComponent::velocity, "velocity">
	);

	RegisterReflectionComponent<Button>(
		"ButtonComponent",
		MemberRegistrationV<&Button::x, "position_x">,
		MemberRegistrationV<&Button::y, "position_y">,
		MemberRegistrationV<&Button::text, "Text">

	);

	// Register HUDComponent anchor enum
	RegisterReflectionComponent<HUDComponent::Anchor>("HUDComponent::Anchor");

	RegisterReflectionComponent<HUDComponent>(
		"HUDComponent",
		MemberRegistrationV<&HUDComponent::m_TextureGuid, "m_TextureGuid">,
		MemberRegistrationV<&HUDComponent::position, "position">,
		MemberRegistrationV<&HUDComponent::size, "size">,
		MemberRegistrationV<&HUDComponent::anchor, "anchor">,
		MemberRegistrationV<&HUDComponent::color, "color">,
		MemberRegistrationV<&HUDComponent::rotation, "rotation">,
		MemberRegistrationV<&HUDComponent::layer, "layer">,
		MemberRegistrationV<&HUDComponent::visible, "visible">
	);

	// Register TextComponent enums
	RegisterReflectionComponent<TextComponent::Anchor>("TextComponent::Anchor");
	RegisterReflectionComponent<TextComponent::Alignment>("TextComponent::Alignment");

	RegisterReflectionComponent<TextComponent>(
		"TextComponent",
		MemberRegistrationV<&TextComponent::m_FontGuid, "fontGuid">,
		MemberRegistrationV<&TextComponent::text, "text">,
		MemberRegistrationV<&TextComponent::position, "position">,
		MemberRegistrationV<&TextComponent::fontSize, "fontSize">,
		MemberRegistrationV<&TextComponent::anchor, "anchor">,
		MemberRegistrationV<&TextComponent::alignment, "alignment">,
		MemberRegistrationV<&TextComponent::lineSpacing, "lineSpacing">,
		MemberRegistrationV<&TextComponent::letterSpacing, "letterSpacing">,
		MemberRegistrationV<&TextComponent::maxWidth, "maxWidth">,
		MemberRegistrationV<&TextComponent::color, "color">,
		MemberRegistrationV<&TextComponent::outlineWidth, "outlineWidth">,
		MemberRegistrationV<&TextComponent::outlineColor, "outlineColor">,
		MemberRegistrationV<&TextComponent::glowStrength, "glowStrength">,
		MemberRegistrationV<&TextComponent::glowColor, "glowColor">,
		MemberRegistrationV<&TextComponent::sdfThreshold, "sdfThreshold">,
		MemberRegistrationV<&TextComponent::smoothing, "smoothing">,
		MemberRegistrationV<&TextComponent::rotation, "rotation">,
		MemberRegistrationV<&TextComponent::layer, "layer">,
		MemberRegistrationV<&TextComponent::visible, "visible">
	);

	// Register TextMeshComponent enums
	RegisterReflectionComponent<TextMeshComponent::BillboardMode>("TextMeshComponent::BillboardMode");
	RegisterReflectionComponent<TextMeshComponent::Alignment>("TextMeshComponent::Alignment");

	RegisterReflectionComponent<TextMeshComponent>(
		"TextMeshComponent",
		MemberRegistrationV<&TextMeshComponent::m_FontGuid, "fontGuid">,
		MemberRegistrationV<&TextMeshComponent::text, "text">,
		MemberRegistrationV<&TextMeshComponent::billboardMode, "billboardMode">,
		MemberRegistrationV<&TextMeshComponent::fontSize, "fontSize">,
		MemberRegistrationV<&TextMeshComponent::referenceDistance, "referenceDistance">,
		MemberRegistrationV<&TextMeshComponent::alignment, "alignment">,
		MemberRegistrationV<&TextMeshComponent::lineSpacing, "lineSpacing">,
		MemberRegistrationV<&TextMeshComponent::letterSpacing, "letterSpacing">,
		MemberRegistrationV<&TextMeshComponent::maxWidth, "maxWidth">,
		MemberRegistrationV<&TextMeshComponent::color, "color">,
		MemberRegistrationV<&TextMeshComponent::outlineWidth, "outlineWidth">,
		MemberRegistrationV<&TextMeshComponent::outlineColor, "outlineColor">,
		MemberRegistrationV<&TextMeshComponent::glowStrength, "glowStrength">,
		MemberRegistrationV<&TextMeshComponent::glowColor, "glowColor">,
		MemberRegistrationV<&TextMeshComponent::sdfThreshold, "sdfThreshold">,
		MemberRegistrationV<&TextMeshComponent::smoothing, "smoothing">,
		MemberRegistrationV<&TextMeshComponent::visible, "visible">
	);
}
