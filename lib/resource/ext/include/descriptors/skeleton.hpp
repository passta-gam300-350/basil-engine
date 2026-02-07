#ifndef RESOURCE_DESCRIPTOR_SKELETON
#define RESOURCE_DESCRIPTOR_SKELETON

#include <rsc-ext/descriptor.hpp>
#include <native/skeleton.h>

struct SkeletonDescriptor {
	rp::descriptor_base base;

	SkeletonResourceData skel;
};

inline SkeletonResourceData CreateSkeleton(SkeletonDescriptor const& skelDesc, std::string const& path = {}, std::string const& serialisedescpath = {}) {
	//SerializeBinary(skelDesc.skel, skelDesc.base.m_guid, ".skeleton", path);
	if (!serialisedescpath.empty())
		rp::serialization::yaml_serializer::serialize(skelDesc, serialisedescpath);
	return skelDesc.skel;
}
RegisterResourceTypeImporter(SkeletonDescriptor, SkeletonResourceData, "skeleton", ".skeleton", CreateSkeleton, "")

#endif