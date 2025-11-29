/******************************************************************************/
/*!
\file   VideoComponent.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2025/11/25
\brief  ECS component for fullscreen video playback (cutscenes)

This component allows playing compiled video resources as fullscreen cutscenes.
Video resources are referenced by GUID and compiled by the resource compiler.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <rsc-core/guid.hpp>
#include "ecs/internal/reflection.h"

// Forward declare video resource type for ResourceRegistry
// This will be defined when your teammate implements the video resource compiler
RegisterResourceTypeForward(VideoResourceData, "video", videodefine)

/**
 * @brief Component for playing fullscreen video cutscenes
 *
 * Usage:
 * - Attach to an entity
 * - Set videoGuid to reference compiled video resource
 * - Call Play() to start playback
 * - Video plays fullscreen, overriding normal rendering
 */
struct VideoComponent
{
    // Resource reference
    rp::BasicIndexedGuid videoGuid{ static_cast<rp::BasicIndexedGuid>(rp::TypeNameGuid<"video">{}) };

    // Playback control
    bool autoPlay = false;          ///< Start playing automatically on scene load
    bool loop = false;              ///< Loop video when it reaches the end
    bool isPlaying = false;         ///< Current playback state (runtime)
    bool isPaused = false;          ///< Pause state (runtime)
    float playbackSpeed = 1.0f;     ///< Playback speed multiplier (1.0 = normal speed)

    // Playback state (runtime, not serialized)
    float currentTime = 0.0f;       ///< Current playback position in seconds
    float duration = 0.0f;          ///< Total video duration in seconds (set by system)

    // Rendering settings
    bool renderFullscreen = true;   ///< Render fullscreen (true) or in-world (false - future feature)
    uint32_t renderLayer = 100;     ///< High render layer to draw on top

    // GPU resources (runtime only, managed by VideoSystem)
    unsigned int textureID = 0;     ///< OpenGL texture ID for current frame
    unsigned int width = 0;         ///< Video width in pixels
    unsigned int height = 0;        ///< Video height in pixels

    // Custom copy constructor to handle runtime-only fields
    VideoComponent() = default;
    VideoComponent(const VideoComponent& other)
        : videoGuid(other.videoGuid)
        , autoPlay(other.autoPlay)
        , loop(other.loop)
        , isPlaying(false)          // Don't copy playback state
        , isPaused(false)
        , playbackSpeed(other.playbackSpeed)
        , currentTime(0.0f)
        , duration(0.0f)
        , renderFullscreen(other.renderFullscreen)
        , renderLayer(other.renderLayer)
        , textureID(0)              // Runtime-only, don't copy
        , width(0)
        , height(0)
    {}

    VideoComponent& operator=(const VideoComponent& other) {
        if (this != &other) {
            videoGuid = other.videoGuid;
            autoPlay = other.autoPlay;
            loop = other.loop;
            playbackSpeed = other.playbackSpeed;
            renderFullscreen = other.renderFullscreen;
            renderLayer = other.renderLayer;
            // Don't copy runtime state
            isPlaying = false;
            isPaused = false;
            currentTime = 0.0f;
            duration = 0.0f;
            textureID = 0;
            width = 0;
            height = 0;
        }
        return *this;
    }

    VideoComponent(VideoComponent&&) = default;
    VideoComponent& operator=(VideoComponent&&) = default;
};

// Reflection registration for serialization
RegisterReflectionTypeBegin(VideoComponent, "VideoComponent")
    MemberRegistrationV<&VideoComponent::videoGuid, "Video GUID">,
    MemberRegistrationV<&VideoComponent::autoPlay, "Auto Play">,
    MemberRegistrationV<&VideoComponent::loop, "Loop">,
    MemberRegistrationV<&VideoComponent::playbackSpeed, "Playback Speed">,
    MemberRegistrationV<&VideoComponent::renderFullscreen, "Render Fullscreen">,
    MemberRegistrationV<&VideoComponent::renderLayer, "Render Layer">
RegisterReflectionTypeEnd
