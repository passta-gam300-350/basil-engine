/******************************************************************************/
/*!
\file   ManagedScreen.hpp
\author Team PASSTA
		Jia Hao Yeo (jiahao.yeo\@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/05
\brief Declaration for screen-related managed bindings (resolution queries).

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef MANAGED_SCREEN_HPP
#define MANAGED_SCREEN_HPP

class ManagedScreen
{
public:
	static int GetWidth();
	static int GetHeight();
};

#endif // MANAGED_SCREEN_HPP
