#ifndef LIB_RESOURCE_CORE_NATIVE_FONT_H
#define LIB_RESOURCE_CORE_NATIVE_FONT_H

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <glm/glm.hpp>
#include "resource/utility.h"
#include "serialization/native_serializer.h"

/// Glyph metrics for a single character in the font atlas
struct GlyphMetrics {
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

/// Kerning pair for improved text spacing
struct KerningPair {
	char32_t first;              // First character codepoint
	char32_t second;             // Second character codepoint
	float kerning_offset;        // Horizontal offset adjustment in pixels
};

/// Font atlas resource data (serialized binary format)
class FontAtlasResourceData {
public:
	// Font metadata
	std::string font_name;                          // Font family name
	std::uint32_t base_font_size;                   // Base font size used for SDF generation
	float ascent;                                   // Distance from baseline to highest point
	float descent;                                  // Distance from baseline to lowest point
	float line_height;                              // Recommended line spacing
	float underline_position;                       // Underline position relative to baseline
	float underline_thickness;                      // Underline thickness

	// SDF parameters
	std::uint32_t sdf_range;                        // SDF distance range in pixels
	bool multi_channel_sdf;                         // Whether atlas uses MSDF (RGB) or SDF (R)

	// Atlas texture data
	Blob atlas_texture_data;                        // DDS compressed texture data

	// Glyph data
	std::vector<GlyphMetrics> glyphs;               // All glyphs in the atlas
	std::unordered_map<char32_t, std::uint32_t> codepoint_to_index; // Fast glyph lookup by codepoint

	// Kerning data (optional)
	std::vector<KerningPair> kerning_pairs;         // Kerning adjustments for character pairs
	std::unordered_map<std::uint64_t, float> kerning_map; // Fast kerning lookup (packed pair -> offset)

	/// Get glyph index from codepoint, returns -1 if not found
	int GetGlyphIndex(char32_t codepoint) const {
		auto it = codepoint_to_index.find(codepoint);
		return (it != codepoint_to_index.end()) ? static_cast<int>(it->second) : -1;
	}

	/// Get kerning offset between two characters, returns 0.0f if no kerning
	float GetKerning(char32_t first, char32_t second) const {
		std::uint64_t key = (static_cast<std::uint64_t>(first) << 32) | second;
		auto it = kerning_map.find(key);
		return (it != kerning_map.end()) ? it->second : 0.0f;
	}
};

#endif
