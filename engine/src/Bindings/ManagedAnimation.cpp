/******************************************************************************/
/*!
\file   ManagedAnimation.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/03/03
\brief This file contains the implementation for the ManagedAnimation class, which
is responsible for managing animation-related functionalities in the managed environment.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#include "Bindings/ManagedAnimation.hpp"
#include <mono/metadata/object.h>
#include "Engine.hpp"
#include "ecs/ecs.h"
#include "Component/AnimationComponent.hpp"
#include "Component/SkeletonComponent.hpp"
#include "Manager/ResourceSystem.hpp"
#include "Animation/Animation.h"
#include "spdlog/spdlog.h"

void ManagedAnimation::Play(uint64_t handle)
{
    ecs::entity entity{ handle };
    if (!entity.all<AnimationComponent>())
    {
        return;
    }
    auto& anim = entity.get<AnimationComponent>();
    if (anim.animatorInstance)
    {
        anim.animatorInstance->play();
        anim.state.isPlaying = true; // sync component state so AnimationSystem doesn't skip
    }
    else
    {
        anim.state.isPlaying = true;
    }
}

void ManagedAnimation::Pause(uint64_t handle)
{
    ecs::entity entity{ handle };
    if (!entity.all<AnimationComponent>())
    {
        return;
    }
    auto& anim = entity.get<AnimationComponent>();
    if (anim.animatorInstance)
    {
        anim.animatorInstance->pause();
    }
    else
    {
        anim.state.isPlaying = false;
    }
}

void ManagedAnimation::Stop(uint64_t handle)
{
    ecs::entity entity{ handle };
    if (!entity.all<AnimationComponent>())
    {
        return;
    }
    auto& anim = entity.get<AnimationComponent>();
    if (anim.animatorInstance)
    {
        anim.animatorInstance->stop();
        anim.state.isPlaying = false; // sync component state so AnimationSystem doesn't skip
        anim.currentTime = 0.0f;
    }
    else
    {
        anim.state.isPlaying = false;
        anim.currentTime = 0.0f;
    }
}

void ManagedAnimation::SetLoop(uint64_t handle, bool loop)
{
    ecs::entity entity{ handle };
    if (!entity.all<AnimationComponent>())
    {
        return;
    }
    auto& anim = entity.get<AnimationComponent>();
    if (anim.animatorInstance)
    {
        anim.animatorInstance->setLoop(loop);
        anim.state.loop = loop; // sync component state so AnimationSystem doesn't overwrite it
    }
    else
    {
        anim.state.loop = loop;
    }
}

void ManagedAnimation::SetPlaybackSpeed(uint64_t handle, float speed)
{
    ecs::entity entity{ handle };
    if (!entity.all<AnimationComponent>())
    {
        return;
    }
    auto& anim = entity.get<AnimationComponent>();
    if (anim.animatorInstance)
    {
        anim.animatorInstance->setPlayBackSpeed(speed);
    }
    else
    {
        anim.state.playbackSpeed = speed;
    }
}

bool ManagedAnimation::IsAnimationFinished(uint64_t handle)
{
    ecs::entity entity{ handle };
    if (!entity.all<AnimationComponent>())
    {
        return false;
    }
    auto& anim = entity.get<AnimationComponent>();
    if (anim.animatorInstance)
    {
        return anim.animatorInstance->isAnimationFinished();
    }
    // Simple animation: finished when non-looping and at/past duration
    return (!anim.state.loop) && (anim.currentTime >= anim.duration);
}

bool ManagedAnimation::PlayAnimation(uint64_t handle, MonoString* animationName, bool shouldLoop)
{
    ecs::entity entity{ handle };
    if (!entity.all<AnimationComponent>())
    {
        return false;
    }
    auto& anim = entity.get<AnimationComponent>();
    
    // If animatorInstance is null, try to initialize it now if we have a skeleton
    if (!anim.animatorInstance && entity.all<SkeletonComponent>())
    {
        spdlog::info("Enter Animation");
        auto& skelComp = entity.get<SkeletonComponent>();
        if (anim.animationdata.m_guid && skelComp.skeletondata.m_guid) {
            spdlog::info("Enter Animation");
            skeleton* skel = ResourceRegistry::Instance().Get<skeleton>(skelComp.skeletondata.m_guid);
            animationContainer* animCont = ResourceRegistry::Instance().Get<animationContainer>(anim.animationdata.m_guid);
            if (skel && animCont) {
                spdlog::info("Enter Animation");
                extern void InitializeSkeletalAnimation(AnimationComponent & animComp, SkeletonComponent & skelComp, const skeleton & skeletonData, animationContainer * animation);
                InitializeSkeletalAnimation(anim, skelComp, *skel, animCont);
            }
        }
    }

    if (anim.animatorInstance) 
    {
        char* nameStr = mono_string_to_utf8(animationName);
        std::string name(nameStr);
        mono_free(nameStr);

        // Check if animator already has this animation
        if (!anim.animatorInstance->hasAnimation(name))
        {
            // Try to find the animation by name in the resource system
            rp::Guid animGuid = ResourceSystem::Instance().GetGuidByName(name);
            if (animGuid)
            {
                // Load the animation and add it to the animator
                animationContainer* newAnim = ResourceRegistry::Instance().Get<animationContainer>(animGuid);
                if (newAnim)
                {
                    anim.animatorInstance->addAnimation(name, newAnim);
                }
            }
        }

        return anim.animatorInstance->playAnimation(name, shouldLoop);
    }
    return false;
}

bool ManagedAnimation::HasAnimation(uint64_t handle, MonoString* animationName)
{
    ecs::entity entity{ handle };
    if (!entity.all<AnimationComponent>())
    {
        return false;
    }
    auto& anim = entity.get<AnimationComponent>();
    if (anim.animatorInstance)
    {
        char* name = mono_string_to_utf8(animationName);
        bool result = anim.animatorInstance->hasAnimation(name);
        mono_free(name);
        return result;
    }
    return false;
}

void ManagedAnimation::SetSpritesheetMode(uint64_t handle, bool enabled)
{
    ecs::entity entity{ handle };
    if (!entity.all<AnimationComponent>())
    {
        return;
    }
    entity.get<AnimationComponent>().isSpritesheetMode = enabled;
}

bool ManagedAnimation::GetSpritesheetMode(uint64_t handle)
{
    ecs::entity entity{ handle };
    if (!entity.all<AnimationComponent>())
    {
        return false;
    }
    return entity.get<AnimationComponent>().isSpritesheetMode;
}
