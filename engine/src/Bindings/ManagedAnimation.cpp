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
#include "Animation/Animation.h"

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
    if (anim.animatorInstance) 
    {
        char* name = mono_string_to_utf8(animationName);
        bool result = anim.animatorInstance->playAnimation(name, shouldLoop);
        mono_free(name);
        return result;
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
