
#include "imxml/widget/WindowWidget.hpp"
#include "imgui.h"
#include "imxml/ImXMLNode.hpp"

void WindowWidget::render(ImXML::ImXMLNode* node)
{
	ImXML::ImXMLNode* n = node;
	ImGui::SetNextWindowPos(ImVec2(posx, posy));
	ImGui::SetNextWindowSize(ImVec2(width, height));
	ImGui::Begin(title.c_str(), &active);
	ImGui::LabelText("%s", title.c_str());

	ImGui::End();
}
