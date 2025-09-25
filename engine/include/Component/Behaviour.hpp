#ifndef BEHAVIOUR_HPP
#define BEHAVIOUR_HPP

#include "Component.hpp"

class Behaviour : public Component
{
	struct ScriptInstance
	{
		UUID<128> scriptClassID;
		uint32_t scriptName;
	};
	std::vector<ScriptInstance> scripts;
	public:
	Behaviour() = default;
	~Behaviour() override = default;
	ComponentType getType() const override;
	bool inEditor() override;

	void AddScript(UUID<128> id, char const* name);
	void RemoveScript(UUID<128> id);
};

#endif // !BEHAVIOUR_HPP