#ifndef LIB_RESOURCE_NATIVE_SERIALIZATION_H
#define LIB_RESOURCE_NATIVE_SERIALIZATION_H

#include <rsc-core/serialization/serializer.hpp>
#include <glm/glm.hpp>

template <>
struct rp::reflection::ExternalTypeMetadata<glm::vec2> {
	using ExternalTypeBinder = ExternalTypeBinderMetadata<glm::vec2, &glm::vec2::x, &glm::vec2::y>;
};

template <>
struct rp::reflection::ExternalTypeMetadata<glm::vec3> {
	using ExternalTypeBinder = ExternalTypeBinderMetadata<glm::vec3, &glm::vec3::x, &glm::vec3::y, &glm::vec3::z>;
};

template <>
struct rp::reflection::ExternalTypeMetadata<glm::vec4> {
	using ExternalTypeBinder = ExternalTypeBinderMetadata<glm::vec4, &glm::vec4::x, &glm::vec4::y, &glm::vec4::z, &glm::vec4::w>;
};

template <>
struct rp::reflection::ExternalTypeMetadata<glm::ivec2> {
	using ExternalTypeBinder = ExternalTypeBinderMetadata<glm::ivec2, &glm::ivec2::x, &glm::ivec2::y>;
};

template <>
struct rp::reflection::ExternalTypeMetadata<glm::ivec3> {
	using ExternalTypeBinder = ExternalTypeBinderMetadata<glm::ivec3, &glm::ivec3::x, &glm::ivec3::y, &glm::ivec3::z>;
};

template <>
struct rp::reflection::ExternalTypeMetadata<glm::ivec4> {
	using ExternalTypeBinder = ExternalTypeBinderMetadata<glm::ivec4, &glm::ivec4::x, &glm::ivec4::y, &glm::ivec4::z, &glm::ivec4::w>;
};

template <typename NativeType>
void SerializeBinary(NativeType const& nativedata, rp::Guid guid, std::string const& ext, std::string const& path = {}) {
	rp::serialization::binary_serializer::serialize(nativedata, rp::utility::resolve_path(path) + "\\" + guid.to_hex() + ext);
}

#endif