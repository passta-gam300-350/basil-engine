/******************************************************************************/
/*!
\file   font.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  Font descriptor definitions

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef RESOURCE_DESCRIPTOR_FONT
#define RESOURCE_DESCRIPTOR_FONT

#include <rsc-ext/descriptor.hpp>
#include "serialization/serializer.h"
#include <string>
#include <vector>

/// Compression format for font atlas texture
enum class FontAtlasCompression : std::uint8_t {
	BC4_R4 = 80u,    // Single channel for SDF (recommended)
	BC3_RGBA8 = 77u, // RGBA for multi-channel SDF or color fonts
	NONE = 0u        // Uncompressed (for debugging)
};

/// Character set preset for font import
enum class CharacterSet : std::uint8_t {
	ASCII,           // Basic ASCII (32-126)
	ASCII_Extended,  // Extended ASCII (32-255)
	Custom           // User-defined character ranges
};

/// Descriptor for font import configuration
struct FontDescriptor {
	rp::descriptor_base base;

	// Atlas generation settings
	std::uint32_t atlas_size{ 1024 };                       // Atlas texture size (power of 2, max 4096)
	std::uint32_t base_font_size{ 64 };                     // Font size for SDF generation (higher = better quality)
	std::uint32_t sdf_range{ 8 };                           // SDF distance range in pixels (4-16 recommended)
	std::uint32_t glyph_padding{ 4 };                       // Padding between glyphs in atlas (prevents bleeding)

	// Character coverage
	CharacterSet character_set{ CharacterSet::ASCII };      // Preset character range
	std::vector<std::pair<std::uint32_t, std::uint32_t>> custom_ranges; // Custom Unicode ranges (start, end) for Custom mode

	// Rendering features
	bool generate_kerning{ true };                          // Include kerning pair data for better spacing
	bool generate_outline_support{ true };                  // Generate extra SDF data for outlines
	bool sdf_multi_channel{ false };                        // Use multi-channel SDF (RGB) for sharper corners

	// Compression
	FontAtlasCompression compression{ FontAtlasCompression::BC4_R4 }; // Atlas texture compression
};

#endif
