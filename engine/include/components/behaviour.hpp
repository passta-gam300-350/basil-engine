/******************************************************************************/
/*!
\file   behaviour.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contains the declaration for the behaviour struct, which
represents the behavior component of an entity in the ECS.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/


#ifndef behaviour_hpp
#define behaviour_hpp

#include <string>

#include "rsc-core/rp.hpp"

struct behaviour
{
	std::vector<std::string> classesName;
	std::vector<rp::Guid> scriptIDs;

};

#endif
