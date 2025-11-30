#ifndef RESOURCE_DESCRIPTOR_ANIMATION
#define RESOURCE_DESCRIPTOR_ANIMATION

#include <rsc-ext/descriptor.hpp>
#include <native/animation.h>

struct AnimationDescriptor {
	rp::descriptor_base base;

	AnimationResourceData anim;
};

inline AnimationResourceData CreateAnimation(AnimationDescriptor const& animDesc, std::string const& path = {}, std::string const& serialisedescpath = {}) {
	SerializeBinary(animDesc.anim, animDesc.base.m_guid, ".animation", path);
	if (!serialisedescpath.empty())
		rp::serialization::yaml_serializer::serialize(animDesc, serialisedescpath);
	return animDesc.anim;
}
RegisterResourceTypeImporter(AnimationDescriptor, AnimationResourceData, "animation", ".animation", CreateAnimation, "")

#endif