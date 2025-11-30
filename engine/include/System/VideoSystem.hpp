/******************************************************************************/
/*!
\file   VideoSystem.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2025/11/25
\brief  System for managing fullscreen video playback and rendering

This system handles video resource loading, frame decoding, texture updates,
and fullscreen rendering for cutscene playback.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#ifndef ENGINE_VIDEO_SYSTEM_H
#define ENGINE_VIDEO_SYSTEM_H
#pragma once

#include "Ecs/ecs.h"
#include "Component/VideoComponent.hpp"
#include "Manager/ResourceSystem.hpp"
#include <Resources/Shader.h>
#include <memory>
#include <glm/glm.hpp>

/**
 * @brief System for managing video playback and rendering
 *
 * Responsibilities:
 * - Load video resources from ResourceRegistry by GUID
 * - Decode video frames and upload to GPU textures
 * - Render fullscreen video quad
 * - Handle playback controls (play/pause/stop/seek)
 */
class VideoSystem : public ecs::SystemBase
{
public:
    VideoSystem();
    ~VideoSystem();

    /**
     * @brief Initialize video system resources
     * Creates fullscreen quad geometry and loads shaders
     */
    void Init();

    /**
     * @brief Update all video components each frame
     * @param world ECS world containing video components
     * @param deltaTime Time since last frame in seconds
     */
    void Update(ecs::world& world, float deltaTime);

    /**
     * @brief Render all active videos
     * @param world ECS world containing video components
     */
    void Render(ecs::world& world);

    /**
     * @brief Cleanup video system resources
     */
    void Shutdown();

    // Playback control methods for specific entities
    void Play(ecs::entity entity);
    void Pause(ecs::entity entity);
    void Stop(ecs::entity entity);
    void Seek(ecs::entity entity, float time);

private:
    /**
     * @brief Create fullscreen quad geometry (NDC coordinates)
     */
    void CreateFullscreenQuad();

    /**
     * @brief Load video resource and initialize component
     * @param videoComp Component to initialize
     * @return true if successfully loaded
     */
    bool LoadVideoResource(VideoComponent& videoComp);

    /**
     * @brief Update video texture with new frame data
     * @param videoComp Component with video data
     * @param deltaTime Time since last frame
     */
    void UpdateVideoFrame(VideoComponent& videoComp, float deltaTime);

    /**
     * @brief Render a single video to fullscreen
     * @param videoComp Component to render
     */
    void RenderVideo(const VideoComponent& videoComp);

    /**
     * @brief Cleanup GPU resources for a video component
     * @param videoComp Component to cleanup
     */
    void CleanupVideoResources(VideoComponent& videoComp);

private:
    // Rendering resources
    unsigned int m_QuadVAO = 0;              ///< Fullscreen quad VAO
    unsigned int m_QuadVBO = 0;              ///< Fullscreen quad VBO
    std::shared_ptr<Shader> m_VideoShader;   ///< Shader for rendering video texture

    bool m_Initialized = false;
};

#endif // ENGINE_VIDEO_SYSTEM_H
