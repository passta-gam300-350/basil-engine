#ifndef RESOURCE_IMPORTER_FONT
#define RESOURCE_IMPORTER_FONT

#include <native/font.h>
#include "descriptors/font.hpp"
#include <DirectXTex.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include <msdfgen.h>
#include <msdfgen-ext.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cassert>

/// Rectangle packer for atlas generation (simple shelf packing)
class RectPacker {
public:
	struct Rect {
		std::uint32_t x, y, width, height;
	};

	RectPacker(std::uint32_t atlas_width, std::uint32_t atlas_height)
		: m_AtlasWidth(atlas_width), m_AtlasHeight(atlas_height), m_CurrentX(0), m_CurrentY(0), m_ShelfHeight(0) {}

	/// Pack a rectangle, returns false if it doesn't fit
	bool Pack(std::uint32_t width, std::uint32_t height, Rect& out_rect) {
		// Try to fit on current shelf
		if (m_CurrentX + width > m_AtlasWidth) {
			// Move to next shelf
			m_CurrentX = 0;
			m_CurrentY += m_ShelfHeight;
			m_ShelfHeight = 0;
		}

		// Check if it fits vertically
		if (m_CurrentY + height > m_AtlasHeight) {
			return false; // Atlas is full
		}

		// Place the rectangle
		out_rect.x = m_CurrentX;
		out_rect.y = m_CurrentY;
		out_rect.width = width;
		out_rect.height = height;

		m_CurrentX += width;
		m_ShelfHeight = std::max(m_ShelfHeight, height);

		return true;
	}

private:
	std::uint32_t m_AtlasWidth, m_AtlasHeight;
	std::uint32_t m_CurrentX, m_CurrentY, m_ShelfHeight;
};

/// Generate character list from descriptor
static std::vector<char32_t> GenerateCharacterList(const FontDescriptor& fontDesc) {
	std::vector<char32_t> characters;

	switch (fontDesc.character_set) {
	case CharacterSet::ASCII:
		for (char32_t c = 32; c <= 126; ++c) {
			characters.push_back(c);
		}
		break;

	case CharacterSet::ASCII_Extended:
		for (char32_t c = 32; c <= 255; ++c) {
			characters.push_back(c);
		}
		break;

	case CharacterSet::Custom:
		for (const auto& range : fontDesc.custom_ranges) {
			for (char32_t c = range.first; c <= range.second; ++c) {
				characters.push_back(c);
			}
		}
		break;
	}

	return characters;
}

/// Font importer implementation
inline FontAtlasResourceData ImportFont(FontDescriptor const& fontDesc) {
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	FontAtlasResourceData fontData;
	fontData.base_font_size = fontDesc.base_font_size;
	fontData.sdf_range = fontDesc.sdf_range;
	fontData.multi_channel_sdf = fontDesc.sdf_multi_channel;

	// Initialize FreeType
	FT_Library ft_library;
	FT_Error error = FT_Init_FreeType(&ft_library);
	assert(!error && "FreeType initialization failed");

	// Load font face
	FT_Face ft_face;
	std::string font_path = rp::utility::resolve_path(fontDesc.base.m_source);
	error = FT_New_Face(ft_library, font_path.c_str(), 0, &ft_face);
	assert(!error && "Failed to load font file");

	// Set font size
	error = FT_Set_Pixel_Sizes(ft_face, 0, fontDesc.base_font_size);
	assert(!error && "Failed to set font size");

	// Extract font metadata
	fontData.font_name = std::string(ft_face->family_name);
	fontData.ascent = static_cast<float>(ft_face->ascender) / 64.0f;
	fontData.descent = static_cast<float>(ft_face->descender) / 64.0f;
	fontData.line_height = static_cast<float>(ft_face->height) / 64.0f;
	fontData.underline_position = static_cast<float>(ft_face->underline_position) / 64.0f;
	fontData.underline_thickness = static_cast<float>(ft_face->underline_thickness) / 64.0f;

	// Create msdfgen FontHandle from FreeType face
	msdfgen::FontHandle* font_handle = msdfgen::adoptFreetypeFont(ft_face);
	assert(font_handle && "Failed to adopt FreeType font");

	// Generate character list
	std::vector<char32_t> characters = GenerateCharacterList(fontDesc);

	// Create atlas image (RGBA for MSDF, single channel for SDF)
	const std::uint32_t channels = fontDesc.sdf_multi_channel ? 4 : 1;
	const std::uint32_t atlas_size = fontDesc.atlas_size;
	std::vector<std::uint8_t> atlas_pixels(atlas_size * atlas_size * channels, 0);

	// Rect packer for atlas layout
	RectPacker packer(atlas_size, atlas_size);

	// Process each character
	for (char32_t codepoint : characters) {
		// Convert FreeType outline to msdfgen shape using FontHandle
		msdfgen::Shape shape;
		double advance = 0;
		if (!msdfgen::loadGlyph(shape, font_handle, codepoint, &advance)) {
			continue; // Failed to load glyph or character not in font
		}

		shape.normalize();

		// Calculate glyph bounds
		msdfgen::Shape::Bounds bounds = shape.getBounds();
		double left = bounds.l;
		double bottom = bounds.b;
		double right = bounds.r;
		double top = bounds.t;

		// Calculate SDF bitmap size (add padding for SDF range)
		const int padding = static_cast<int>(fontDesc.sdf_range) + static_cast<int>(fontDesc.glyph_padding);
		int glyph_width = static_cast<int>(std::ceil(right - left)) + padding * 2;
		int glyph_height = static_cast<int>(std::ceil(top - bottom)) + padding * 2;

		// Handle invisible glyphs (space, etc.) - they need metrics but minimal atlas space
		if (glyph_width <= 0 || glyph_height <= 0) {
			glyph_width = 1;   // Minimal size for atlas (won't render visible pixels)
			glyph_height = 1;
		}

		// Pack glyph into atlas
		RectPacker::Rect rect;
		if (!packer.Pack(glyph_width, glyph_height, rect)) {
			assert(false && "Atlas is too small for all glyphs");
			continue;
		}

		// Generate SDF bitmap (separate paths for compile-time template parameters)
		double range = static_cast<double>(fontDesc.sdf_range);
		msdfgen::Vector2 translate(padding - left, padding - bottom);
		const double scale = 1.0;

		if (fontDesc.sdf_multi_channel) {
			// Multi-channel SDF (RGBA: better quality for complex glyphs)
			msdfgen::Bitmap<float, 3> msdf_bitmap(glyph_width, glyph_height);
			msdfgen::edgeColoringSimple(shape, 3.0);
			msdfgen::generateMSDF(msdf_bitmap, shape, range, scale, translate);

			// Copy to atlas (RGB -> RGBA8)
			for (int y = 0; y < glyph_height; ++y) {
				for (int x = 0; x < glyph_width; ++x) {
					const int atlas_x = rect.x + x;
					const int atlas_y = rect.y + y;
					const int atlas_index = (atlas_y * atlas_size + atlas_x) * 4;
					atlas_pixels[atlas_index + 0] = static_cast<std::uint8_t>(std::clamp(msdf_bitmap(x, y)[0] * 255.0f, 0.0f, 255.0f));
					atlas_pixels[atlas_index + 1] = static_cast<std::uint8_t>(std::clamp(msdf_bitmap(x, y)[1] * 255.0f, 0.0f, 255.0f));
					atlas_pixels[atlas_index + 2] = static_cast<std::uint8_t>(std::clamp(msdf_bitmap(x, y)[2] * 255.0f, 0.0f, 255.0f));
					atlas_pixels[atlas_index + 3] = 255; // Full alpha
				}
			}
		}
		else {
			// Single-channel SDF
			msdfgen::Bitmap<float, 1> sdf_bitmap(glyph_width, glyph_height);
			msdfgen::generateSDF(sdf_bitmap, shape, range, scale, translate);

			// Copy to atlas (R -> R8)
			for (int y = 0; y < glyph_height; ++y) {
				for (int x = 0; x < glyph_width; ++x) {
					const int atlas_x = rect.x + x;
					const int atlas_y = rect.y + y;
					const int atlas_index = atlas_y * atlas_size + atlas_x;
					atlas_pixels[atlas_index] = static_cast<std::uint8_t>(std::clamp(sdf_bitmap(x, y)[0] * 255.0f, 0.0f, 255.0f));
				}
			}
		}

		// Store glyph metrics
		GlyphMetrics metrics;
		metrics.codepoint = codepoint;
		metrics.uv_min = glm::vec2(static_cast<float>(rect.x) / atlas_size, static_cast<float>(rect.y) / atlas_size);
		metrics.uv_max = glm::vec2(static_cast<float>(rect.x + rect.width) / atlas_size, static_cast<float>(rect.y + rect.height) / atlas_size);
		metrics.size = glm::vec2(static_cast<float>(glyph_width), static_cast<float>(glyph_height));
		// Bearing: offset from cursor to top-left of glyph bitmap
		// X: horizontal offset (left edge of outline minus padding)
		// Y: negated for Y-down screen coordinates (negative = above baseline)
		metrics.bearing = glm::vec2(
			static_cast<float>(left) - padding,
			-(static_cast<float>(top) + padding)
		);
		metrics.advance = static_cast<float>(advance);
		metrics.sdf_scale = 1.0f / static_cast<float>(fontDesc.sdf_range);

		fontData.glyphs.push_back(metrics);
		fontData.codepoint_to_index[codepoint] = static_cast<std::uint32_t>(fontData.glyphs.size() - 1);
	}

	// Extract kerning pairs if requested
	if (fontDesc.generate_kerning && FT_HAS_KERNING(ft_face)) {
		for (size_t i = 0; i < characters.size(); ++i) {
			for (size_t j = 0; j < characters.size(); ++j) {
				FT_UInt first_index = FT_Get_Char_Index(ft_face, characters[i]);
				FT_UInt second_index = FT_Get_Char_Index(ft_face, characters[j]);

				FT_Vector kerning;
				FT_Get_Kerning(ft_face, first_index, second_index, FT_KERNING_DEFAULT, &kerning);

				if (kerning.x != 0) {
					KerningPair pair;
					pair.first = characters[i];
					pair.second = characters[j];
					pair.kerning_offset = static_cast<float>(kerning.x) / 64.0f;

					fontData.kerning_pairs.push_back(pair);

					std::uint64_t key = (static_cast<std::uint64_t>(pair.first) << 32) | pair.second;
					fontData.kerning_map[key] = pair.kerning_offset;
				}
			}
		}
	}

	// Cleanup msdfgen and FreeType
	msdfgen::destroyFont(font_handle);
	FT_Done_Face(ft_face);
	FT_Done_FreeType(ft_library);

	// Compress atlas to DDS format
	DirectX::Image raw_image;
	raw_image.width = atlas_size;
	raw_image.height = atlas_size;
	raw_image.format = (channels == 4) ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R8_UNORM;
	raw_image.rowPitch = atlas_size * channels;
	raw_image.slicePitch = atlas_size * atlas_size * channels;
	raw_image.pixels = atlas_pixels.data();

	DirectX::ScratchImage compressed_image;
	DXGI_FORMAT target_format = (fontDesc.compression == FontAtlasCompression::BC4_R4) ? DXGI_FORMAT_BC4_UNORM :
	                            (fontDesc.compression == FontAtlasCompression::BC3_RGBA8) ? DXGI_FORMAT_BC3_UNORM :
	                            raw_image.format;

	if (fontDesc.compression != FontAtlasCompression::NONE) {
		[[maybe_unused]] HRESULT hr = DirectX::Compress(raw_image, target_format, DirectX::TEX_COMPRESS_PARALLEL, DirectX::TEX_THRESHOLD_DEFAULT, compressed_image);
		assert(!FAILED(hr) && "Atlas compression failed");
	}
	else {
		compressed_image.InitializeFromImage(raw_image);
	}

	// Save to DDS blob
	DirectX::Blob dds_blob;
	[[maybe_unused]] HRESULT hr = DirectX::SaveToDDSMemory(compressed_image.GetImages(), compressed_image.GetImageCount(), compressed_image.GetMetadata(), DirectX::DDS_FLAGS_NONE, dds_blob);
	assert(!FAILED(hr) && "Failed to save DDS");

	fontData.atlas_texture_data.AllocateExact(dds_blob.GetBufferSize());
	std::memcpy(fontData.atlas_texture_data.Raw(), dds_blob.GetConstBufferPointer(), dds_blob.GetBufferSize());

	return fontData;
}

RegisterResourceTypeImporter(FontDescriptor, FontAtlasResourceData, "font_atlas", ".font", ImportFont, ".ttf", ".otf")

#endif
