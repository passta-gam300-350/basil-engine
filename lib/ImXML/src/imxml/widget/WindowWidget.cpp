/******************************************************************************/
/*!
\file   WindowWidget.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Window widget implementation

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "imxml/widget/WindowWidget.hpp"
#include "imgui.h"
#include "imxml/ImXMLNode.hpp"

void WindowWidget::render(ImXML::ImXMLNode*)
{
	//ImXML::ImXMLNode* n = node;
	ImGui::SetNextWindowPos(ImVec2(posx, posy));
	ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width), static_cast<float>(height)));
	ImGui::Begin(title.c_str(), &active);
	ImGui::LabelText("%s", title.c_str());

	ImGui::End();
}
