#ifndef RELATONSHIPCOMPONENT_HPP
#define RELATONSHIPCOMPONENT_HPP
#include "Component.hpp"

class RelationshipComponent : public Component
{
	ecs::entity parentHandle;

public:
	RelationshipComponent() = default;
	RelationshipComponent(ecs::entity parent) : parentHandle{ parent } {}
	~RelationshipComponent() override = default;
	ComponentType getType() const override;
	ecs::entity getParent() const;
	void setParent(ecs::entity newParent);

	bool inEditor() override;

};


#endif //RELATONSHIPCOMPONENT_HPP
