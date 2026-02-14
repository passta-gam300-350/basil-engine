/******************************************************************************/
/*!
\file   animation.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Animation descriptor definitions

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef RESOURCE_DESCRIPTOR_ANIMATION
#define RESOURCE_DESCRIPTOR_ANIMATION

#include <rsc-ext/descriptor.hpp>
#include <native/animation.h>

struct AnimationDescriptor {
	rp::descriptor_base base;

	AnimationResourceData anim;
};

inline AnimationResourceData CreateAnimation(AnimationDescriptor const& animDesc, [[maybe_unused]] std::string const& path = {}, std::string const& serialisedescpath = {}) {
	//SerializeBinary(animDesc.anim, animDesc.base.m_guid, ".animation", path);
	if (!serialisedescpath.empty())
		rp::serialization::yaml_serializer::serialize(animDesc, serialisedescpath);
	return animDesc.anim;
}
RegisterResourceTypeImporter(AnimationDescriptor, AnimationResourceData, "animation", ".animation", CreateAnimation, "")

#endif