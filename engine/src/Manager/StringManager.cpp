/******************************************************************************/
/*!
\file   StringManager.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  String manager implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "Manager/StringManager.hpp"
#include <functional>






uint32_t StringManager::HashString(std::string const& name)
{
	uint32_t hash = 2166136261u;
	for (char c : name) {
		hash ^= static_cast<uint8_t>(c);
		hash *= 16777619u;
	}
	return hash;

}


StringManager& StringManager::GetInstance()
{
	static StringManager instance{};
	return instance;
}


StringManager::StringID StringManager::Intern_String(std::string&& str)
{
	auto it = StringTable.find(HashString(str));
	if (it != StringTable.end())
	{
		return it->first;
	}
	else
	{
		offset_t  offset = static_cast<offset_t>(container.size());
		container.append(str);
		StringID id = HashString(str);
		StringTable[id] = offset;
		return id;
	}
}

std::string_view StringManager::Get_String(StringID id)
{
	auto it = StringTable.find(id);
	if (it != StringTable.end())
	{
		offset_t offset = it->second;
		return std::string_view(container.data() + offset);
	}
	return {};
}
