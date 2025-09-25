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