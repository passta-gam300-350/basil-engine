/******************************************************************************/
/*!
\file   IWidget.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Widget interface

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef IWIDGET_HPP
#define IWIDGET_HPP
#include <string>

namespace ImXML
{
	class ImXMLNode;
}

class IWidget {
protected:
	std::string _name;
	bool active;
public:

	IWidget(std::string name);
	virtual ~IWidget() = default;

	virtual void render(ImXML::ImXMLNode* node) = 0;

	virtual bool GetActive() const { return active; }

};

#endif // IWIDGET_HPP