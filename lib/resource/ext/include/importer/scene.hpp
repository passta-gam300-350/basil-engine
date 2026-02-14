/******************************************************************************/
/*!
\file   scene.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Scene importer

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
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