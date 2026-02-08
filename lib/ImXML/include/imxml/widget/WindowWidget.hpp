/******************************************************************************/
/*!
\file   WindowWidget.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Window widget class

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef WINDOW_WIDGET_HPP
#define WINDOW_WIDGET_HPP

#include "IWidget.hpp"

class WindowWidget : public IWidget
{
	float posx, posy;
	int width;
	int height;
	std::string title;
public:
	WindowWidget(std::string name, float x, float y, int w, int h, std::string t);
	virtual ~WindowWidget() = default;
	void render(ImXML::ImXMLNode* node) override;

	bool GetActive() const override { return active; }	
};
#endif // WINDOW_WIDGET_HPP