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
#include <glm/common.hpp>

void animationSystem::Update(ecs::world& world, float dt)
{
	// SKELETAL ANIMATION //
	auto skeletalEntities = world.filter_entities<AnimationComponent, SkeletonComponent>();
	for (auto eachAEntity : skeletalEntities)
	{
		auto& animationComponent = eachAEntity.get<AnimationComponent>();
		auto& skeletonComponent = eachAEntity.get<SkeletonComponent>();

		// Scene-loaded entities do not serialize the runtime-only isSkeletalAnim flag,
		// so infer skeletal usage from the presence of a live animator or valid asset refs.
		bool shouldUseSkeletalPath =
			animationComponent.isSkeletalAnim ||
			animationComponent.animatorInstance != nullptr ||
			(animationComponent.animationdata.m_guid && skeletonComponent.skeletondata.m_guid);
		if (!shouldUseSkeletalPath)
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
				animationContainer* animCont = ResourceRegistry::Instance().Get<animationContainer>(animationComponent.animationdata.m_guid);
				if (skel && animCont) {
					InitializeSkeletalAnimation(animationComponent, skeletonComponent, *skel, animCont);
				}
			}
			continue;
		}
		if (!animationComponent.animationdata.m_guid || !skeletonComponent.skeletondata.m_guid) {
			delete animationComponent.animatorInstance;
			animationComponent.animatorInstance = nullptr;
			continue;
		}
		if (animationComponent.lastKnownAnimGuid != animationComponent.animationdata.m_guid) {
			CleanupSkeletalAnimation(animationComponent);
			skeleton* skel = ResourceRegistry::Instance().Get<skeleton>(skeletonComponent.skeletondata.m_guid);
			InitializeSkeletalAnimation(animationComponent, skeletonComponent, *skel, ResourceRegistry::Instance().Get<animationContainer>(animationComponent.animationdata.m_guid));
			continue;
		}

		animator* anim = animationComponent.animatorInstance;
		skeleton& skel = skeletonComponent.skeletonData;

		// Sync component state to animator so editor changes take effect
		anim->state = animationComponent.state;

		anim->updateAnimation(dt, skel);

		// Sync back so animator changes (e.g. non-loop finished) reflect in editor
		animationComponent.state = anim->state;
		animationComponent.currentTime = anim->currentTime;

		skeletonComponent.finalBoneMatrices = anim->finalBoneMatrices;
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
		[[maybe_unused]] auto& transformComponent = eachAEntity.get<TransformComponent>();
		if (!animationComponent.state.isPlaying)
		{
			continue;
		}
		if (!animationComponent.animationdata.m_guid)
		{
			continue;
		}
		if (!eachAEntity.all<AnimationBoneChannelTransformComponent>()) {
			eachAEntity.add<AnimationBoneChannelTransformComponent>();
		}
		AnimationBoneChannelTransformComponent& boneTransformComponent = eachAEntity.get<AnimationBoneChannelTransformComponent>();
		animationContainer* cont_ptr = ResourceRegistry::Instance().Get<animationContainer>(animationComponent.animationdata.m_guid);

		if (!cont_ptr) {
			//warn
			continue;
		}

		animationContainer& cont = *cont_ptr;

		animationComponent.duration = cont_ptr->duration;
		animationComponent.ticksPerSecond = cont_ptr->ticksPerSecond;


		if (animationComponent.duration <= 0.0f || animationComponent.ticksPerSecond <= 0.0f)
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

		if (boneTransformComponent.bone_trans.size() != cont.channels.size()) {
			boneTransformComponent.bone_trans.clear();
		}
		
		for (auto& bchannel : cont.channels) {
			bchannel.update(animationComponent.currentTime);
			boneTransformComponent.bone_trans[bchannel.getID()] = bchannel.getLocalTransform();
		}
	}
}

void FreeAnimatorOnDestroy([[maybe_unused]] entt::registry& registry, entt::entity entity) {
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
	ac.name = animData.m_name; // Set the clip name from resource
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

	// Pre-load all clips FIRST to force any ResourcePool<animationContainer> vector
	// reallocations before we cache raw pointers. Each Get<> on a new GUID calls
	// AllocateSlot() which may call m_Slots.emplace_back(), invalidating all previously
	// returned T* pointers. The 'animation' parameter is dangling after reallocation.
	for (auto& [clipName, clipGuid] : animComp.animationClips)
	{
		if (!clipGuid.m_guid) continue;
		ResourceRegistry::Instance().Get<animationContainer>(clipGuid.m_guid);
	}

	// Re-fetch main animation pointer now that all reallocations are done.
	animationContainer* freshAnim = ResourceRegistry::Instance().Get<animationContainer>(animComp.animationdata.m_guid);
	if (!freshAnim) freshAnim = animation; // fallback; shouldn't happen
	if (!freshAnim)
	{
		return;
	}

    // Use actual animation name instead of hardcoded string
    std::string animName = freshAnim->name.empty() ? "default" : freshAnim->name;
	animComp.animatorInstance->addAnimation(animName, freshAnim);
	animComp.animatorInstance->playAnimation(animName, animComp.state.loop);

	// Add all clips - pointers are stable now (no further pool allocations will occur)
	for (auto& [clipName, clipGuid] : animComp.animationClips)
	{
		if (!clipGuid.m_guid) continue;
		animationContainer* clipCont = ResourceRegistry::Instance().Get<animationContainer>(clipGuid.m_guid);
		if (clipCont)
		{
			animComp.animatorInstance->addAnimation(clipName, clipCont);
		}
	}

	// 3. Set animation
	animComp.animatorInstance->currentAnimation = freshAnim;
	animComp.animatorInstance->currentAnimationName = animName;
	animComp.currentAnimationContainer = freshAnim;
	animComp.duration = freshAnim->duration;
	animComp.ticksPerSecond = freshAnim->ticksPerSecond;
	animComp.animatorInstance->state = animComp.state;
	if (freshAnim->duration > 0.0f)
	{
		if (animComp.state.loop)
		{
			animComp.animatorInstance->currentTime = fmod(animComp.currentTime, freshAnim->duration);
			if (animComp.animatorInstance->currentTime < 0.0f)
			{
				animComp.animatorInstance->currentTime += freshAnim->duration;
			}
		}
		else
		{
			animComp.animatorInstance->currentTime = glm::clamp(animComp.currentTime, 0.0f, freshAnim->duration);
		}
	}
	else
	{
		animComp.animatorInstance->currentTime = 0.0f;
	}
	animComp.currentTime = animComp.animatorInstance->currentTime;

	// 4. Enable skeletal mode
	animComp.isSkeletalAnim = true;

	// Record the GUID so AnimationSystem can detect future editor asset swaps
	animComp.lastKnownAnimGuid = animComp.animationdata.m_guid;
}

void CleanupSkeletalAnimation(AnimationComponent& animComp)
{
	if (animComp.animatorInstance)
	{
		delete animComp.animatorInstance;
		animComp.animatorInstance = nullptr;
	}
}

REGISTER_RESOURCE_TYPE_ALIASE(boneChannel, animation, LoadBoneChannel, [](boneChannel&) {return; })
REGISTER_RESOURCE_TYPE_ALIASE(animationContainer, animationcont, LoadAnimationContainer, [](animationContainer&) {return; })
REGISTER_RESOURCE_TYPE_ALIASE(skeleton, skeleton, LoadSkeleton, [](skeleton&) {return; })
