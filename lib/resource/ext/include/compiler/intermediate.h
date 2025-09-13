#ifndef LIB_RESOURCE_INTERMEDIATE_H
#define LIB_RESOURCE_INTERMEDIATE_H

#include <native/model.h>

namespace Resource {
	MeshResource load_mesh(std::string const&);
	ModelResource load_model(std::string const&);
}

#endif