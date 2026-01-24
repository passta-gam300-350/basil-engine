/******************************************************************************/
/*!
\file   FontAtlas.h
\author Claude Code
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Declares font atlas data structures and loading utilities for SDF text rendering

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <cstdint>

/// Glyph metrics for runtime text rendering
struct GlyphData {
	char32_t codepoint;          // Unicode codepoint

	// Atlas UV coordinates (normalized 0-1)
	glm::vec2 uv_min;            // Top-left UV
	glm::vec2 uv_max;            // Bottom-right UV

	// Glyph dimensions in pixels (at base font size)
	glm::vec2 size;              // Width and height of glyph

	// Glyph positioning
	glm::vec2 bearing;           // Offset from baseline to top-left of glyph
	float advance;               // Horizontal advance to next glyph

	// SDF parameters
	float sdf_scale;             // Scale factor for SDF distance field
};

/// Runtime font atlas resource (GPU-side)
class FontAtlas {
public:
	FontAtlas() = default;
	~FontAtlas();

	// Non-copyable but movable
	FontAtlas(const FontAtlas&) = delete;
	FontAtlas& operator=(const FontAtlas&) = delete;
	FontAtlas(FontAtlas&& other) noexcept;
	FontAtlas& operator=(FontAtlas&& other) noexcept;

	/// Get OpenGL texture ID for the font atlas
	unsigned int GetTextureID() const { return m_AtlasTextureID; }

	/// Get font metadata
	const std::string& GetFontName() const { return m_FontName; }
	std::uint32_t GetBaseFontSize() const { return m_BaseFontSize; }
	float GetAscent() const { return m_Ascent; }
	float GetDescent() const { return m_Descent; }
	float GetLineHeight() const { return m_LineHeight; }

	/// Get glyph data by codepoint, returns nullptr if not found
	const GlyphData* GetGlyph(char32_t codepoint) const;

	/// Get kerning offset between two characters, returns 0.0f if no kerning
	float GetKerning(char32_t first, char32_t second) const;

	/// Check if font has a specific glyph
	bool HasGlyph(char32_t codepoint) const;

	/// Get SDF parameters
	std::uint32_t GetSDFRange() const { return m_SDFRange; }
	bool IsMultiChannelSDF() const { return m_MultiChannelSDF; }

	// Public members (following Texture pattern for resource pipeline integration)
	// GPU data
	unsigned int m_AtlasTextureID = 0;

	// Font metadata
	std::string m_FontName;
	std::uint32_t m_BaseFontSize = 0;
	float m_Ascent = 0.0f;
	float m_Descent = 0.0f;
	float m_LineHeight = 0.0f;
	float m_UnderlinePosition = 0.0f;
	float m_UnderlineThickness = 0.0f;

	// SDF parameters
	std::uint32_t m_SDFRange = 0;
	bool m_MultiChannelSDF = false;

	// Glyph data
	std::vector<GlyphData> m_Glyphs;
	std::unordered_map<char32_t, std::uint32_t> m_CodepointToIndex;

	// Kerning data
	std::unordered_map<std::uint64_t, float> m_KerningMap; // Packed pair -> offset
};

/// Font atlas loader (GPU resource creation, decoupled from resource pipeline)
class FontAtlasLoader {
public:
	/// Create GPU texture from DDS data (separated concern - no resource pipeline dependency)
	static unsigned int CreateGPUAtlasTexture(const void* dds_data, std::size_t dds_size);
};
