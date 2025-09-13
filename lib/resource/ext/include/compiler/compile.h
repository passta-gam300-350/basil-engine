#ifndef LIB_RESOURCE_COMPILE_H
#define LIB_RESOURCE_COMPILE_H

#include "descriptors/descriptors.h"
#include <string>

namespace Resource {

	void set_output_dir(std::string const&);
	void set_output_dir(std::wstring const&);
	std::string const get_output_dir();
	std::wstring const& get_output_dir_wstr();
	void compile_geometry(ResourceDescriptor const&);
	void compile_model(ResourceDescriptor const&);
	void compile_textures(ResourceDescriptor const&);
	void compile_descriptor(ResourceDescriptor const&);

}

#endif