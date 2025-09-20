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