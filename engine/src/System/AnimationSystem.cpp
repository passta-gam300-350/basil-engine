/******************************************************************************/
/*!
\file   AnimationSystem.cpp
\author Team PASSTA
		Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/28
\brief  Defines Animation system

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include "System/AnimationSystem.hpp"
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Engine.hpp"
#include "Component/AnimationComponent.hpp"
#include "Manager/ResourceSystem.hpp"
#include "Component/SkeletonComponent.hpp"

void animationSystem::FixedUpdate(ecs::world& world)
{
	float dt = Engine::GetDeltaTime();
	// SKELETAL ANIMATION //
	auto skeletalEntities = world.filter_entities<AnimationComponent, SkeletonComponent>();
	for (auto eachAEntity : skeletalEntities)
	{
		auto& animationComponent = eachAEntity.get<AnimationComponent>();
		auto& skeletonComponent = eachAEntity.get<SkeletonComponent>();

		// Skip if not skeletal or not playing
		if (animationComponent.isSkeletalAnim == false && animationComponent.animatorInstance)
		{
			continue;
		}
		if (animationComponent.state.isPlaying == false)
		{
			continue;
		}
		if (animationComponent.animatorInstance == nullptr)
		{
			//what if animation changed
			if (animationComponent.animationdata.m_guid && skeletonComponent.skeletondata.m_guid) {
				skeleton* skel = ResourceRegistry::Instance().Get<skeleton>(skeletonComponent.skeletondata.m_guid);
				InitializeSkeletalAnimation(animationComponent, skeletonComponent, *skel, ResourceRegistry::Instance().Get<animationContainer>(animationComponent.animationdata.m_guid));
			}
			continue;
		}
		if (!animationComponent.animationdata.m_guid || !skeletonComponent.skeletondata.m_guid) {
			delete animationComponent.animatorInstance;
			animationComponent.animatorInstance = nullptr;
			continue;
		}
		if (animationComponent.animatorInstance->currentAnimation != ResourceRegistry::Instance().Get<animationContainer>(animationComponent.animationdata.m_guid)) {
			CleanupSkeletalAnimation(animationComponent);
			skeleton* skel = ResourceRegistry::Instance().Get<skeleton>(skeletonComponent.skeletondata.m_guid);
			InitializeSkeletalAnimation(animationComponent, skeletonComponent, *skel, ResourceRegistry::Instance().Get<animationContainer>(animationComponent.animationdata.m_guid));
			continue;
		}

		animator* anim = animationComponent.animatorInstance;
		skeleton& skel = skeletonComponent.skeletonData;
		anim->updateAnimation(dt, skel);
		skeletonComponent.finalBoneMatrices = anim->finalBoneMatrices;
		int i{};
	}
	// SIMPLE ANIMATION //
	auto simpleAnimationEntites = world.filter_entities<AnimationComponent, TransformComponent>(ecs::exclude<SkeletonComponent>);
	for (auto eachAEntity : simpleAnimationEntites)
	{
		auto& animationComponent = eachAEntity.get<AnimationComponent>();
		if (animationComponent.isSkeletalAnim == true)
		{
			continue;
		}
		auto& transformComponent = eachAEntity.get<TransformComponent>();
		if (!animationComponent.state.isPlaying)
		{
			continue;
		}
		if (!animationComponent.animationdata.m_guid)
		{
			continue;
		}
		if (animationComponent.duration <= 0.0f)
		{
			continue;
		}
		animationComponent.currentTime += animationComponent.ticksPerSecond * dt * animationComponent.state.playbackSpeed;
		if (animationComponent.currentTime > animationComponent.duration)
		{
			if (animationComponent.state.loop == true)
			{
				animationComponent.currentTime = fmod(animationComponent.currentTime, animationComponent.duration);
			}
			else
			{
				animationComponent.currentTime = animationComponent.duration;
				animationComponent.state.isPlaying = false;
			}
		}
		boneChannel* bchannel = ResourceRegistry::Instance().Get<boneChannel>(animationComponent.animationdata.m_guid);
		bchannel->update(animationComponent.currentTime);
		glm::mat4 localMatrix = bchannel->getLocalTransform();
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(localMatrix, scale, rotation, translation, skew, perspective);
		transformComponent.m_Translation = translation;
		transformComponent.m_Rotation = glm::degrees(glm::eulerAngles(rotation));
		transformComponent.m_Scale = scale;
		transformComponent.isDirty = true;
	}
}

void FreeAnimatorOnDestroy(entt::registry& registry, entt::entity entity) {
	ecs::entity const ecsEntity = Engine::GetWorld().impl.entity_cast(entity);
	auto& anc = ecsEntity.get<AnimationComponent>();
	CleanupSkeletalAnimation(anc);
}

void animationSystem::Init()
{
	auto world = Engine::GetWorld();
	entt::registry& registry = world.impl.get_registry();

	registry.on_construct<AnimationComponent>().disconnect();
	registry.on_destroy<AnimationComponent>().disconnect();
	registry.on_destroy<AnimationComponent>().connect<FreeAnimatorOnDestroy>();
}

void animationSystem::Exit()
{
	auto world = Engine::GetWorld();
	auto animationen = world.filter_entities<AnimationComponent>();
	for (auto e : animationen) {
		e.remove<AnimationComponent>();
	}
}

boneChannel LoadBoneChannel(const char* data) {
	AnimationResourceData animResData = rp::serialization::serializer<"bin">::deserialize<AnimationResourceData>(
		reinterpret_cast<const std::byte*>(data)
	);
	AnimationResourceData::Channel const& animData = animResData.m_channels[0];
	boneChannel bc{ animData.m_name, animData.m_id};
	for (auto const& poskey : animData.m_positions) {
		bc.addPositionKeyframe(poskey.first / 1000, poskey.second);
	}
	for (auto const& rotkey : animData.m_rotations) {
		bc.addRotationKeyframe(rotkey.first / 1000, rotkey.second);
	}
	for (auto const& sclkey : animData.m_scales) {
		bc.addScaleKeyframe(sclkey.first / 1000, sclkey.second);
	}
	return bc;
}

skeleton LoadSkeleton(const char* data) {
	SkeletonResourceData skelData = rp::serialization::serializer<"bin">::deserialize<SkeletonResourceData>(
		reinterpret_cast<const std::byte*>(data)
	);
	skeleton skel{};
	skel.bones.reserve(skelData.m_bones.size());
	for (auto bone : skelData.m_bones) {
		oneSkeletonBone onebone;
		onebone.id = bone.m_id;
		onebone.name = bone.m_bone_name;
		onebone.parentIndex = bone.m_parent_index;
		onebone.inverseBind = glm::mat4(bone.m_inv_bind_c1, bone.m_inv_bind_c2, bone.m_inv_bind_c3, bone.m_inv_bind_c4);
		skel.bones.emplace_back(onebone);
	}
	return skel;
}

animationContainer LoadAnimationContainer(const char* data) {
	AnimationResourceData animData = rp::serialization::serializer<"bin">::deserialize<AnimationResourceData>(
		reinterpret_cast<const std::byte*>(data)
	);
	animationContainer ac{};
	ac.channels.reserve(animData.m_channels.size());
	for (AnimationResourceData::Channel chl : animData.m_channels) {
		boneChannel bc{ chl.m_name, chl.m_id };
		for (auto const& poskey : chl.m_positions) {
			bc.addPositionKeyframe(poskey.first, poskey.second);
		}
		for (auto const& rotkey : chl.m_rotations) {
			bc.addRotationKeyframe(rotkey.first, rotkey.second);
		}
		for (auto const& sclkey : chl.m_scales) {
			bc.addScaleKeyframe(sclkey.first, sclkey.second);
		}
		ac.channels.emplace_back(bc);
	}
	ac.ticksPerSecond = animData.m_ticks_per_sec;
	ac.duration = animData.m_duration;
	return ac;
}

// function / API to call when add component / load animation data 
void InitializeSkeletalAnimation(AnimationComponent& animComp, SkeletonComponent& skelComp, const skeleton& skeletonData, animationContainer* animation)
{
	// 1. Set up skeleton
	skelComp.skeletonData = skeletonData;
	skelComp.InitializeMatrices();

	// 2. Create animator
	int boneCount = static_cast<int>(skeletonData.bones.size());
	if (animComp.animatorInstance) {
		delete animComp.animatorInstance;
		animComp.animatorInstance = nullptr;
	}
	animComp.animatorInstance = new animator(boneCount);

	animComp.animatorInstance->addAnimation("default_ani1", animation);
	animComp.animatorInstance->playAnimation("default_ani1", true);

	// 3. Set animation
	animComp.animatorInstance->currentAnimation = animation;
	animComp.duration = animation->duration;
	animComp.ticksPerSecond = animation->ticksPerSecond;

	// 4. Enable skeletal mode
	animComp.isSkeletalAnim = true;
}

void CleanupSkeletalAnimation(AnimationComponent& animComp)
{
	if (animComp.animatorInstance)
	{
		delete animComp.animatorInstance;
		animComp.animatorInstance = nullptr;
	}
}

REGISTER_RESOURCE_TYPE_ALIASE(boneChannel, animation, LoadBoneChannel, [](boneChannel& bc) {return; })
REGISTER_RESOURCE_TYPE_ALIASE(animationContainer, animationcont, LoadAnimationContainer, [](animationContainer& ac) {return; })
REGISTER_RESOURCE_TYPE_ALIASE(skeleton, skeleton, LoadSkeleton, [](skeleton& ac) {return; })