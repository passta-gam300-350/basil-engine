#ifndef RESOURCE_IMPORTER_SCENE
#define RESOURCE_IMPORTER_SCENE

#include <native/scene.h>
#include "descriptors/scene.hpp"

inline SceneResourceData ImportScene(SceneDescriptor const& scnDesc) {
	YAML::Node nd{ YAML::LoadFile(rp::utility::resolve_path(texDesc.base.m_source)) };
	nd["scene"]["guid"] = scnDesc.base.m_guid.to_hex();
	YAML::Emitter emitter{};
	emitter << nd;
	SceneResourceData scndata;
	scndata.scene_data.AllocateExact(emitter.size());
	std::memcpy(scndata.scene_data.Raw(), emitter.c_str(), emitter.size());
	return scndata;
}

RegisterResourceTypeImporter(SceneDescriptor, SceneResourceData, "scene", ".scenedata", ImportScene, ".scene")

#endif