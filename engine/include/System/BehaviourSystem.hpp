#pragma once
#include <mono/metadata/object-forward.h>

#include "ecs/system/system.h"
#include "uuid/uuid.hpp"


class BehaviourSystem : public ecs::SystemBase
{
	std::unordered_map<ecs::entity, bool> enabledStates;
	std::unordered_map<UUID<128>, MonoObject*> scriptClasses;
public:
	void Init() override;
	void Update(ecs::world&, float dt) override;
	void FixedUpdate(ecs::world&) override;
	void Exit() override;


};
