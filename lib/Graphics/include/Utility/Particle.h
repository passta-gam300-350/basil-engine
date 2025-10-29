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
enum class BlendMode
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