#pragma once

#include <string>
#include <cstdint>

class Texture
{
public:
	enum class Format
	{
		None = 0,
		RGB, RGBA, RED, RG, BGR, BGRA, DEPTH, DEPTH_STENCIL
	};

	enum class FilterMode
	{
		Linear, Nearest, 
		LinearMipmapLinear, LinearMipmapNearest,
		NearestMipmapLinear, NearestMipmapNearest
	};

	enum class WrapMode
	{
		Repeat,
		ClampToEdge,
		ClampToBorder,
		MirroredRepeat
	};

	enum class Type
	{
		Diffuse, Specular, Normal, Height, 
		Ambient, Emissive,Shininess, Opacity,
		Displacement, Lightmap, Reflection,
		BaseColor, Metallic, Roughness, AmbientOcclusion, // PBR
		Unknown
	};

	struct Properties
	{
		Format format = Format::RGBA;
		FilterMode filterMode = FilterMode::Linear;
		WrapMode wrapMode = WrapMode::Repeat;
		bool generateMipmaps = true;
		bool flipOnLoad = true;
		Type type = Type::Diffuse;
	};

	// Constructors
	Texture(uint32_t width, uint32_t height, Properties const &props = {});
	Texture(std::string const &path, Properties const &props = {});
	~Texture();

	// Core Functions
	void SetData(void *data, uint32_t size);
	void Bind(uint32_t textureUnit = 0) const;
	void Unbind() const;

	// Getters
	uint32_t GetWidth() const
	{
		return m_Width;
	}
	uint32_t GetHeight() const
	{
		return m_Height;
	}
	uint32_t GetChannels() const
	{
		return m_Channels;
	}
	uint32_t GetTextureID() const
	{
		return m_TextureID;
	}

private:
	void CreateTexture(const void *data);
	void SetTextureParameters();

	std::string m_Path;
	uint32_t m_Width = 0;
	uint32_t m_Height = 0;
	uint32_t m_Channels = 0;
	uint32_t m_TextureID = 0;
	Properties m_Properties;
};