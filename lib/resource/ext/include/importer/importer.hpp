#ifndef LIB_RESOURCE_IMPORTER_H
#define LIB_RESOURCE_IMPORTER_H

#include "descriptors/descriptors.hpp"

//handles importation to native formats
namespace Resource {
	void ImportModel(ResourceDescriptor&);
	void ImportTexture(ResourceDescriptor&);
	void Import(ResourceDescriptor&);
}

#endif