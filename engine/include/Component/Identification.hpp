/******************************************************************************/
/*!
\file   Identification.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Identification component for entity naming

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
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