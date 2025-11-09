#pragma once
#include <mono/metadata/object-forward.h>

#include "ecs/system/system.h"
#include "rsc-core/rp.hpp"



class BehaviourSystem : public ecs::SystemBase
{

	std::unordered_map<uint64_t, bool> enabledStates;

public:
	bool isActive = true;
	enum struct CollisionCallback
	{
		OnCollisionEnter,
		OnCollisionStay,
		OnCollisionExit
	} ;
	static BehaviourSystem& Instance();
	void Init() override;
	void Update(ecs::world&, float dt) override;
	void FixedUpdate(ecs::world&) override;
	void Exit() override;



	void AddClass(const char* name, const char* klassNamespace = "");

	bool AddScriptToEntityComponent(ecs::entity& entity, ecs::world& world, rp::Guid scriptID);


	void AddScriptToEntityComponent(ecs::entity& entity, ecs::world& world, const char* klassname, const char* klass_ns="");


	void RegisterComponent(ecs::entity& entity);


	void OnCollisionCallback(ecs::entity& entity, ecs::entity other, CollisionCallback callback);

};

RegisterSystemDerivedPreUpdate(BehaviourSystem, BehaviourSystem, (ecs::ReadSet<ecs::entity>), (ecs::EmptySet), 60);
