#ifndef IdentificationComponent_HPP
#define IdentificationComponent_HPP
#include "Component.hpp"

class Identifier : Component
{

	uint32_t identifier;
	uuid::UUID<128> uuid;

public:


	Identifier();
	Identifier(std::string&& name);
	bool inEditor() override;
	ComponentType getType() const override;

	~Identifier() override = default;


	void setName(std::string&& name);
	std::string getName();


};

#endif