#ifndef LIB_RESOURCE_DESCRIPTORS_H
#define LIB_RESOURCE_DESCRIPTORS_H

#include "descriptors/mesh.hpp"
#include "descriptors/material.hpp"
#include "descriptors/audio.hpp"
#include "descriptors/video.hpp"
#include "descriptors/texture.hpp"
#include "descriptors/animation.hpp"
#include "descriptors/font.hpp"

inline rp::Guid GetOldGuidOrGenerate(rp::descriptor_base base, std::string const& filepath = "") {
	std::string descfilename = filepath.empty() ? base.m_name + ".desc" : filepath;
	rp::Guid guid = std::filesystem::exists(descfilename) ? rp::serialization::yaml_serializer::deserialize<rp::descriptor_base>(descfilename).m_guid : rp::Guid::generate();
	return guid ? guid : rp::Guid::generate();
}


template <typename Descriptor>
inline void SaveOrOverwriteDescriptors(Descriptor& desc, std::string parent) {
	std::string descfilenpath = std::filesystem::path(parent + "/" + desc.base.m_name + ".desc").lexically_normal().string();
	desc.base.m_guid = GetOldGuidOrGenerate(desc.base, descfilenpath);
	rp::serialization::yaml_serializer::serialize(desc, descfilenpath);
}

#endif