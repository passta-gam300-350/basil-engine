/******************************************************************************/
/*!
\file   ProjectItemList.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Project item list panel

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef PROJECTITEMPANEL_HPP
#define PROJECTITEMPANEL_HPP

#include <string>

#include "IMGuiPanel.hpp"

class ProjectItemList : public IMGuiPanel
{

public:
	void Prepare() override;
	void Render() override;
	void Cleanup() override;
	ProjectItemList() = default;

	~ProjectItemList() override = default;
};

#endif // PROJECTITEMLIST_HPP