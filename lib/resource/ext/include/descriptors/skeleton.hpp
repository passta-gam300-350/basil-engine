/******************************************************************************/
/*!
\file   skeleton.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Skeleton descriptor definitions

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef RESOURCE_DESCRIPTOR_SKELETON
#define RESOURCE_DESCRIPTOR_SKELETON

#include <rsc-ext/descriptor.hpp>
#include <native/skeleton.h>

struct SkeletonDescriptor {
	rp::descriptor_base base;

	SkeletonResourceData skel;
};

inline SkeletonResourceData CreateSkeleton(SkeletonDescriptor const& skelDesc, [[maybe_unused]] std::string const& path = {}, std::string const& serialisedescpath = {}) {
	//SerializeBinary(skelDesc.skel, skelDesc.base.m_guid, ".skeleton", path);
	if (!serialisedescpath.empty())
		rp::serialization::yaml_serializer::serialize(skelDesc, serialisedescpath);
	return skelDesc.skel;
}
RegisterResourceTypeImporter(SkeletonDescriptor, SkeletonResourceData, "skeleton", ".skeleton", CreateSkeleton, "")

#endif