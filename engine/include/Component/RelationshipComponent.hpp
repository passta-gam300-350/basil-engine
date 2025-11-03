#ifndef RELATONSHIPCOMPONENT_HPP
#define RELATONSHIPCOMPONENT_HPP
#include "Component.hpp"
#include <vector>

class RelationshipComponent : public Component
{
	ecs::entity parentHandle;
	std::vector<ecs::entity> childrenHandles;

public:
	RelationshipComponent() = default;
	RelationshipComponent(ecs::entity parent) : parentHandle{ parent } {}
	~RelationshipComponent() override = default;
	ComponentType getType() const override;

	// Parent management
	ecs::entity getParent() const;
	void setParent(ecs::entity newParent);
	bool hasParent() const;

	// Children management
	const std::vector<ecs::entity>& getChildren() const;
	void addChild(ecs::entity child);
	void removeChild(ecs::entity child);
	void clearChildren();
	size_t getChildCount() const;

	bool inEditor() override;

};


#endif //RELATONSHIPCOMPONENT_HPP
