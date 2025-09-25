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