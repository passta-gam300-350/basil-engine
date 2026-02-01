/******************************************************************************/
/*!
\file   Particle.h
\author Team PASSTA
		Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/11/07
\brief  Data structures for particle simulation and rendering

Defines the core particle data structures used throughout the particle system:
- Particle: CPU-side particle with physics properties (position, velocity, lifetime)
- ParticleInstanceData: GPU-side SSBO layout for instanced rendering (48 bytes)
- ParticleRenderData: Submission format for particle systems to the renderer
- BlendMode: Blend mode enumeration (alpha, additive, multiply, opaque)

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <cstdint>

struct Texture; // forward declaration, save compile time

/**
 * @brief Blend modes for particle rendering
 *
 * Controls how particles blend with the background.
 */
enum class BlendMode : std::uint8_t
{
    Alpha,        // standard transparency (smoke, clouds, dust)
    Additive,     // additive blending (fire, sparks, magic)
    Multiply,     // multiplicative blending (shadows, darkness)
    Opaque        // no blending (solid particles - rarely used)
};

struct Particle 
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec4 color = glm::vec4(1.0f);
    float size = 1.0f;
    float rotation = 0.0f;
    float lifeTime = 0.0f;
    float maxLifeTime = 1.0f;
    bool active = false;
};

struct ParticleInstanceData 
{
    glm::vec3 position;
    float size;
    glm::vec4 color;
    float rotation;      
    uint32_t textureID;
    uint32_t padding[2]; // padding, make it 48 bytes 
};

struct ParticleRenderData // submit to the renderer, then it will convert to the format needed for ssbo
{
    std::vector<Particle> particles;
    std::shared_ptr<Texture> texture;
    BlendMode blendMode = BlendMode::Alpha;
    bool depthWrite = false;
    uint32_t renderLayer = 10;
};