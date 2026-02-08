/******************************************************************************/
/*!
\file   IMGuiPanel.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  ImGui panel base class

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef IMGuiPanel_HPP
#define IMGuiPanel_HPP

class IMGuiPanel
{
public:
	virtual void Prepare() = 0;
	virtual void Render() = 0;
	virtual void Cleanup() = 0;
	virtual ~IMGuiPanel() = default;
};

#endif // IMGuiPanel_HPP