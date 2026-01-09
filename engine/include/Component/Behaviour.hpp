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