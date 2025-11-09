/******************************************************************************/
/*!
\file   MonoTypeRegistry.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief This file contains the declaration for the MonoTypeRegistry class, which
is responsible for registering and retrieving Mono type information
based on type names and script IDs.
Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef MONO_TYPE_REGISTRY_HPP
#define MONO_TYPE_REGISTRY_HPP
#include "rsc-core/rp.hpp"
#include <unordered_map>
class MonoTypeRegistry {
	using ScriptID = rp::Guid;
	std::unordered_map<uint32_t, ScriptID> m_TypeMap;

public:
	void Register(std::string typeName, ScriptID id);
	void Unregister(std::string typeName);
	ScriptID GetMonoEntityID(std::string name) const;
	void Exit();
};


#endif