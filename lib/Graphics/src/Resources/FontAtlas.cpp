/******************************************************************************/
/*!
\file   FontAtlas.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Implementation of font atlas loading and GPU resource management

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include <Resources/FontAtlas.h>
#include <glad/glad.h>
#include <spdlog/spdlog.h>
#define TINYDDSLOADER_IMPLEMENTATION
#include <tinyddsloader.h>
#include <cstring>

// FontAtlas implementation
FontAtlas::~FontAtlas() {
	if (m_AtlasTextureID != 0) {
		glDeleteTextures(1, &m_AtlasTextureID);
		m_AtlasTextureID = 0;
	}
}

FontAtlas::FontAtlas(FontAtlas&& other) noexcept
	: m_AtlasTextureID(other.m_AtlasTextureID),
	  m_FontName(std::move(other.m_FontName)),
	  m_BaseFontSize(other.m_BaseFontSize),
	  m_Ascent(other.m_Ascent),
	  m_Descent(other.m_Descent),
	  m_LineHeight(other.m_LineHeight),
	  m_UnderlinePosition(other.m_UnderlinePosition),
	  m_UnderlineThickness(other.m_UnderlineThickness),
	  m_SDFRange(other.m_SDFRange),
	  m_MultiChannelSDF(other.m_MultiChannelSDF),
	  m_Glyphs(std::move(other.m_Glyphs)),
	  m_CodepointToIndex(std::move(other.m_CodepointToIndex)),
	  m_KerningMap(std::move(other.m_KerningMap))
{
	other.m_AtlasTextureID = 0;
	other.m_BaseFontSize = 0;
	other.m_Ascent = 0.0f;
	other.m_Descent = 0.0f;
	other.m_LineHeight = 0.0f;
	other.m_SDFRange = 0;
	other.m_MultiChannelSDF = false;
}

FontAtlas& FontAtlas::operator=(FontAtlas&& other) noexcept {
	if (this != &other) {
		// Clean up existing resources
		if (m_AtlasTextureID != 0) {
			glDeleteTextures(1, &m_AtlasTextureID);
		}

		// Move data
		m_AtlasTextureID = other.m_AtlasTextureID;
		m_FontName = std::move(other.m_FontName);
		m_BaseFontSize = other.m_BaseFontSize;
		m_Ascent = other.m_Ascent;
		m_Descent = other.m_Descent;
		m_LineHeight = other.m_LineHeight;
		m_UnderlinePosition = other.m_UnderlinePosition;
		m_UnderlineThickness = other.m_UnderlineThickness;
		m_SDFRange = other.m_SDFRange;
		m_MultiChannelSDF = other.m_MultiChannelSDF;
		m_Glyphs = std::move(other.m_Glyphs);
		m_CodepointToIndex = std::move(other.m_CodepointToIndex);
		m_KerningMap = std::move(other.m_KerningMap);

		// Reset source
		other.m_AtlasTextureID = 0;
		other.m_BaseFontSize = 0;
		other.m_Ascent = 0.0f;
		other.m_Descent = 0.0f;
		other.m_LineHeight = 0.0f;
		other.m_SDFRange = 0;
		other.m_MultiChannelSDF = false;
	}
	return *this;
}

const GlyphData* FontAtlas::GetGlyph(char32_t codepoint) const {
	auto it = m_CodepointToIndex.find(codepoint);
	if (it != m_CodepointToIndex.end() && it->second < m_Glyphs.size()) {
		return &m_Glyphs[it->second];
	}
	return nullptr;
}

float FontAtlas::GetKerning(char32_t first, char32_t second) const {
	std::uint64_t key = (static_cast<std::uint64_t>(first) << 32) | second;
	auto it = m_KerningMap.find(key);
	return (it != m_KerningMap.end()) ? it->second : 0.0f;
}

bool FontAtlas::HasGlyph(char32_t codepoint) const {
	return m_CodepointToIndex.find(codepoint) != m_CodepointToIndex.end();
}

// FontAtlasLoader implementation (GPU resource creation only)
unsigned int FontAtlasLoader::CreateGPUAtlasTexture(const void* dds_data, std::size_t dds_size) {
	if (!dds_data || dds_size == 0) {
		spdlog::error("FontAtlasLoader: Invalid DDS data");
		return 0;
	}

	// Load DDS file
	tinyddsloader::DDSFile ddsFile;
	auto result = ddsFile.Load(static_cast<const uint8_t*>(dds_data), dds_size);
	if (result != tinyddsloader::Result::Success) {
		spdlog::error("FontAtlasLoader: Failed to load DDS data");
		return 0;
	}

	// Map DDS format to OpenGL format
	GLenum glCompressedFormat = 0;
	switch (ddsFile.GetFormat()) {
	case tinyddsloader::DDSFile::DXGIFormat::BC4_UNorm:
		glCompressedFormat = GL_COMPRESSED_RED_RGTC1; // Single-channel SDF
		break;
	case tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm:
		glCompressedFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; // Multi-channel SDF
		break;
	case tinyddsloader::DDSFile::DXGIFormat::R8_UNorm:
		glCompressedFormat = GL_R8; // Uncompressed single-channel
		break;
	case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UNorm:
		glCompressedFormat = GL_RGBA8; // Uncompressed RGBA
		break;
	default:
		spdlog::error("FontAtlasLoader: Unsupported DDS format");
		return 0;
	}

	// Create OpenGL texture
	unsigned int textureID = 0;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Upload compressed texture data
	const auto* imageData = ddsFile.GetImageData(0, 0);
	if (!imageData) {
		spdlog::error("FontAtlasLoader: Failed to get DDS image data");
		glDeleteTextures(1, &textureID);
		return 0;
	}

	if (ddsFile.GetFormat() == tinyddsloader::DDSFile::DXGIFormat::BC4_UNorm ||
		ddsFile.GetFormat() == tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm) {
		// Compressed format
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, glCompressedFormat,
			static_cast<GLsizei>(ddsFile.GetWidth()),
			static_cast<GLsizei>(ddsFile.GetHeight()),
			0,
			static_cast<GLsizei>(imageData->m_memSlicePitch),
			imageData->m_mem);
	}
	else {
		// Uncompressed format
		GLenum format = (ddsFile.GetFormat() == tinyddsloader::DDSFile::DXGIFormat::R8_UNorm) ? GL_RED : GL_RGBA;
		glTexImage2D(GL_TEXTURE_2D, 0, glCompressedFormat,
			static_cast<GLsizei>(ddsFile.GetWidth()),
			static_cast<GLsizei>(ddsFile.GetHeight()),
			0, format, GL_UNSIGNED_BYTE, imageData->m_mem);
	}

	// Set texture parameters (linear filtering, clamp to edge for text)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	return textureID;
}
