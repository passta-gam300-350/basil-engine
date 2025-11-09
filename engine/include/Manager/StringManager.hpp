/******************************************************************************/
/*!
\file   StringManager.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contains the declaration for the StringManager class, which
provides functionality for managing and interning strings.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef StringManager_HPP
#define StringManager_HPP
#include <cstdint>
#include <string>
#include <unordered_map>
class StringManager
{
public:
	
	using StringID = uint32_t;
	using offset_t = uint32_t;
private:
	std::string container{};
	std::unordered_map<StringID, offset_t> StringTable;

	uint32_t HashString(std::string const& name);
public:
	

	static StringManager& GetInstance();
	StringID Intern_String(std::string&& str);
	std::string_view Get_String(StringID id);
	void Clear();
	
};

#endif