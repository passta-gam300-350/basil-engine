#ifndef IdentificationComponent_HPP
#define IdentificationComponent_HPP
#include "Component.hpp"

class IdentifierComponent : Component
{

	uint32_t identifier;
	UUID<128> uuid;

public:


	IdentifierComponent();
	IdentifierComponent(std::string&& name);
	bool inEditor() override;
	ComponentType getType() const override;

	~IdentifierComponent() override = default;


	void setName(std::string&& name);
	std::string getName();


};

#endif