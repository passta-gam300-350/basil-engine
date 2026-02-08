/******************************************************************************/
/*!
\file   BehaviourSystem.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Behaviour system for script execution

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once
#include <mono/metadata/object-forward.h>
#include <string>

#include "ecs/system/system.h"
#include "rsc-core/rp.hpp"
#include <components/behaviour.hpp>



class BehaviourSystem : public ecs::SystemBase
{

	std::unordered_map<uint64_t, bool> enabledStates;

public:
	bool isActive = false;
	bool firstRun = true;
	bool unloaded = false;
	enum struct CollisionCallback
	{
		OnCollisionEnter,
		OnCollisionStay,
		OnCollisionExit,
		OnTriggerEnter,
		OnTriggerStay,
		OnTriggerExit


	} ;
	static BehaviourSystem& Instance();
	void Init() override;
	void Reload();
	void Update(ecs::world&, float dt) override;
	void FixedUpdate(ecs::world&) override;
	void Exit() override;



	void AddClass(const char* name, const char* klassNamespace = "");

	bool AddScriptToEntityComponent(ecs::entity& entity, ecs::world& world, rp::Guid scriptID);


	void AddScriptToEntityComponent(ecs::entity& entity, ecs::world& world, const char* klassname, const char* klass_ns="");
	void ApplyScriptProperties(behaviour& component, const std::string& managedName, rp::Guid scriptID);


	void RegisterComponent(ecs::entity& entity);


	void OnCollisionCallback(ecs::entity& entity, ecs::entity other, CollisionCallback callback);

	rp::Guid GetScriptIDFromClassName(ecs::entity& entity ,const char* name, const char* ns=nullptr);



};

RegisterSystemDerivedPreUpdate(BehaviourSystem, BehaviourSystem, (ecs::ReadSet<ecs::entity>), (ecs::EmptySet), 60);
