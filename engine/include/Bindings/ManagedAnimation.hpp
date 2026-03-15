/******************************************************************************/
/*!
\file   ManagedAnimation.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/03/03
\brief This file contains the declaration for the ManagedAnimation class, which
is responsible for managing animation-related functionalities in the managed environment.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef MANAGED_ANIMATION_HPP
#define MANAGED_ANIMATION_HPP

#include <cstdint>

typedef struct _MonoString MonoString;

class ManagedAnimation
{
public:
    static void Play(uint64_t handle);
    static void Pause(uint64_t handle);
    static void Stop(uint64_t handle);
    static void SetLoop(uint64_t handle, bool loop);
    static void SetPlaybackSpeed(uint64_t handle, float speed);
    static bool IsAnimationFinished(uint64_t handle);
    static bool PlayAnimation(uint64_t handle, MonoString* animationName, bool shouldLoop);
    static bool HasAnimation(uint64_t handle, MonoString* animationName);
    static void SetSpritesheetMode(uint64_t handle, bool enabled);
    static bool GetSpritesheetMode(uint64_t handle);
};

#endif // MANAGED_ANIMATION_HPP
