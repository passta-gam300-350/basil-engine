/******************************************************************************/
/*!
\file   System.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Base system class declaration

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef SYSTEM_HPP
#define SYSTEM_HPP

namespace ecs
{
	struct world;
}

class System
{
	virtual void Init() = 0;
	virtual void Update(ecs::world&) = 0;
	virtual void FixedUpdate(ecs::world&) = 0;
	virtual void Exit() = 0;
	virtual ~System() = default;
};


#endif //!SYSTEM_HPP