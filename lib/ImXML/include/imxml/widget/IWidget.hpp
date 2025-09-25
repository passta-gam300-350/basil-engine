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