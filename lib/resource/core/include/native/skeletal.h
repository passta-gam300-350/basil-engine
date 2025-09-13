#ifndef LIB_RESOURCE_CORE_NATIVE_SKELETAL_H
#define LIB_RESOURCE_CORE_NATIVE_SKELETAL_H

#include <glm/glm.hpp>

namespace Resource {
	struct SkeletalResource {
		struct SkeletalNodes {
			std::int32_t m_parent;
			glm::mat4 m_local;
			glm::mat4 m_trans;
		};

		std::vector<SkeletalNodes> m_nodes;
		std::vector<std::uint32_t> m_bone_node;
		std::vector<glm::mat4> m_inverse_bind;
	};
}

#endif