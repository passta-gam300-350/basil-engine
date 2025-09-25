#ifndef LIB_RES_LOADER
#define LIB_RES_LOADER

#include "native/mesh.h"
#include "native/model.h"
#include "native/texture.h"
#include "native/material.h"
#include "native/shader.h"

namespace Resource {
	MeshAssetData load_native_mesh_from_memory(const char* data);
	MeshAssetData load_native_mesh_from_file(std::string const& file_path);
	MaterialAssetData load_native_material_from_memory(const char* data);
	MaterialAssetData load_native_material_from_file(std::string const& file_path);
	ShaderAssetData load_native_shader_from_memory(const char* data);
	ShaderAssetData load_native_shader_from_file(std::string const& file_path);
}

#endif 