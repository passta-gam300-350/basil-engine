/******************************************************************************/
/*!
\file   Behaviour.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Behaviour component for scripting integration

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef BEHAVIOUR_HPP
#define BEHAVIOUR_HPP

#include "Component.hpp"

class Behaviour : public Component
{
	struct ScriptInstance
	{
		uuid::UUID<128> scriptClassID;
		uint32_t scriptName;
	};
	std::vector<ScriptInstance> scripts;
	public:
	Behaviour() = default;
	~Behaviour() override = default;
	ComponentType getType() const override;
	bool inEditor() override;

	void AddScript(uuid::UUID<128> id, char const* name);
	void RemoveScript(uuid::UUID<128> id);
};

#endif // !BEHAVIOUR_HPP