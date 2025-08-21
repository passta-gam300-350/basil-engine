#include <Resources/Texture.h>
#include <glad/gl.h>
#include <iostream>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Utils
{
	static GLenum TextureFormatToGL(Texture::Format format)
	{
		switch (format)
		{
			case Texture::Format::RGB: return GL_RGB;
			case Texture::Format::RGBA: return GL_RGBA;
			case Texture::Format::RED: return GL_RED;
			case Texture::Format::RG: return GL_RG;
			case Texture::Format::BGR: return GL_BGR;
			case Texture::Format::BGRA: return GL_BGRA;
			case Texture::Format::DEPTH: return GL_DEPTH_COMPONENT;
			case Texture::Format::DEPTH_STENCIL: return GL_DEPTH_STENCIL;
			default: return GL_RGBA;
		}
	}

	static GLint FilterModeToGL(Texture::FilterMode mode)
	{
		switch (mode)
		{
			case Texture::FilterMode::Linear: return GL_LINEAR;
			case Texture::FilterMode::Nearest: return GL_NEAREST;
			case Texture::FilterMode::LinearMipmapLinear: return GL_LINEAR_MIPMAP_LINEAR;
			case Texture::FilterMode::LinearMipmapNearest: return GL_LINEAR_MIPMAP_NEAREST;
			case Texture::FilterMode::NearestMipmapLinear: return GL_NEAREST_MIPMAP_LINEAR;
			case Texture::FilterMode::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST;
			default: return GL_LINEAR;
		}
	}

	static GLint WrapModeToGL(Texture::WrapMode mode)
	{
		switch (mode)
		{
			case Texture::WrapMode::Repeat: return GL_REPEAT;
			case Texture::WrapMode::ClampToEdge: return GL_CLAMP_TO_EDGE;
			case Texture::WrapMode::ClampToBorder: return GL_CLAMP_TO_BORDER;
			case Texture::WrapMode::MirroredRepeat: return GL_MIRRORED_REPEAT;
			default: return GL_REPEAT;
		}
	}

	static bool HasMipmapFilter(Texture::FilterMode mode)
	{
		return mode == Texture::FilterMode::LinearMipmapLinear ||
			mode == Texture::FilterMode::LinearMipmapNearest ||
			mode == Texture::FilterMode::NearestMipmapLinear ||
			mode == Texture::FilterMode::NearestMipmapNearest;
	}

}

Texture::Texture(uint32_t width, uint32_t height, Properties const &props = {})
	: m_Width(width), m_Height(height), m_Properties(props)
{
	// Determine channels from format
	switch (props.format)
	{
		case Format::RED: m_Channels = 1; break;
		case Format::RG: m_Channels = 2; break;
		case Format::RGB: m_Channels = 3; break;
		case Format::BGR: m_Channels = 3; break;
		case Format::RGBA: m_Channels = 4; break;
		case Format::BGRA: m_Channels = 4; break;
		default: m_Channels = 4; break;
	}

	CreateTexture(nullptr);
}

Texture::Texture(std::string const &path, Properties const &props = {})
	: m_Path(path), m_Properties(props)
{
	// Check if file exists
	if (!std::filesystem::exists(path))
	{
		std::cerr << "Texture file not found: " << path << std::endl;
		return;
	}

	// STB image options
	stbi_set_flip_vertically_on_load(props.flipOnLoad);

	// Load image
	int width, height, channels;
	unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, 0);

	if (!data)
	{
		std::cerr << "Failed to load texture: " << path << std::endl;
		std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;
		return;
	}

	m_Width = static_cast<uint32_t>(width);
	m_Height = static_cast<uint32_t>(height);
	m_Channels = static_cast<uint32_t>(channels);

	// Auto-detect format if not specified
	if (props.format == Format::None)
	{
		m_Properties.format = GetFormatFromChannels(channels);
	}

	// Create the OpenGL texture
	CreateTexture(data);

	// Free image data
	stbi_image_free(data);

	std::cout << "Loaded texture: " << path << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;
}

Texture::~Texture()
{
	if (m_TextureID != 0)
	{
		glDeleteTextures(1, &m_TextureID);
	}
}

void Texture::SetData(void *data, uint32_t size)
{
	uint32_t bytesPerPixel = m_Channels;
	uint32_t expectedSize = m_Width * m_Height * bytesPerPixel;

	if (size != expectedSize)
	{
		std::cerr << "Data size doesn't match texture dimensions! Expected: "
			<< expectedSize << ", Got: " << size << std::endl;
		return;
	}

	glBindTexture(GL_TEXTURE_2D, m_TextureID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height,
		Utils::TextureFormatToGL(m_Properties.format),
		GL_UNSIGNED_BYTE, data);

	if (m_Properties.generateMipmaps)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::Bind(uint32_t textureUnit = 0) const
{
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);
}

void Texture::Unbind() const
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

std::string Texture::TypeToString(Type type)
{
	switch (type)
	{
		case Type::Diffuse:           return "diffuse";
		case Type::Specular:          return "specular";
		case Type::Normal:            return "normal";
		case Type::Height:            return "height";
		case Type::Ambient:           return "ambient";
		case Type::Emissive:          return "emissive";
		case Type::Shininess:         return "shininess";
		case Type::Opacity:           return "opacity";
		case Type::Displacement:      return "displacement";
		case Type::Lightmap:          return "lightmap";
		case Type::Reflection:        return "reflection";
		case Type::BaseColor:         return "basecolor";
		case Type::Metallic:          return "metallic";
		case Type::Roughness:         return "roughness";
		case Type::AmbientOcclusion:  return "ao";
		default:                      return "unknown";
	}
}

Texture::Type Texture::StringToType(const std::string &typeStr)
{
	std::string lower = typeStr;
	std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

	if (lower == "diffuse" || lower == "albedo")          return Type::Diffuse;
	if (lower == "specular")                              return Type::Specular;
	if (lower == "normal" || lower == "normalmap")        return Type::Normal;
	if (lower == "height" || lower == "heightmap")        return Type::Height;
	if (lower == "ambient")                               return Type::Ambient;
	if (lower == "emissive" || lower == "emission")       return Type::Emissive;
	if (lower == "shininess")                             return Type::Shininess;
	if (lower == "opacity" || lower == "alpha")           return Type::Opacity;
	if (lower == "displacement")                          return Type::Displacement;
	if (lower == "lightmap")                              return Type::Lightmap;
	if (lower == "reflection")                            return Type::Reflection;
	if (lower == "basecolor" || lower == "base_color")    return Type::BaseColor;
	if (lower == "metallic" || lower == "metalness")      return Type::Metallic;
	if (lower == "roughness")                             return Type::Roughness;
	if (lower == "ao" || lower == "ambientocclusion")     return Type::AmbientOcclusion;

	return Type::Unknown;
}

Texture::Format Texture::GetFormatFromChannels(int channels)
{
	switch (channels)
	{
		case 1: return Format::RED;
		case 2: return Format::RG;
		case 3: return Format::RGB;
		case 4: return Format::RGBA;
		default: return Format::RGBA;
	}
}

void Texture::CreateTexture(const void *data)
{
	// Generate texture
	glGenTextures(1, &m_TextureID);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);

	// Set Texture Params
	SetTextureParameters();

	// Upload texture data
	GLenum internalFormat = Utils::TextureFormatToGL(m_Properties.format);
	GLenum dataFormat = internalFormat;

	// Handle different data formats for upload
	if (m_Properties.format == Format::BGR)
	{
		dataFormat = GL_BGR;
		internalFormat = GL_RGB;
	}
	else if (m_Properties.format == Format::BGRA)
	{
		dataFormat = GL_BGRA;
		internalFormat = GL_RGBA;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0,
		dataFormat, GL_UNSIGNED_BYTE, data);

	// Generate mipmaps if requested and filter supports it
	if (m_Properties.generateMipmaps && 
		(Utils::HasMipmapFilter(m_Properties.filterMode) || 
		data != nullptr))
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::SetTextureParameters()
{
	GLint minFilter = Utils::FilterModeToGL(m_Properties.filterMode);
	GLint magFilter = (m_Properties.filterMode == FilterMode::Nearest ||
		m_Properties.filterMode == FilterMode::NearestMipmapLinear ||
		m_Properties.filterMode == FilterMode::NearestMipmapNearest) ?
		GL_NEAREST : GL_LINEAR;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

	GLint wrapMode = Utils::WrapModeToGL(m_Properties.wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

	// Set border color for clamp to border
	if (m_Properties.wrapMode == WrapMode::ClampToBorder)
	{
		float borderColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	}
}