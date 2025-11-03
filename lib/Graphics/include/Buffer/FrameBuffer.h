/******************************************************************************/
/*!
\file   FrameBuffer.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the FrameBuffer class and related specifications for OpenGL FBO management

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include <cstdint>
#include <vector>

enum class FBOTextureFormat
{
	None = 0,
	RGBA8,
	RGB8,           // 8-bit RGB (LDR, no alpha, linear)
	SRGB8,          // 8-bit RGB (LDR, no alpha, sRGB - automatic gamma correction)
	SRGB8_ALPHA8,   // sRGB format with alpha for automatic gamma correction
	RGBA16F,        // 16-bit floating point for G-buffer precision
	RGB16F,         // 16-bit float per channel, no alpha (HDR) ← ogldev uses this
	RGB11F_G11F_B10F, // Packed HDR format for bloom (32 bits total: 11+11+10)
	RED_INTEGER,
	DEPTH24STENCIL8,
	Depth = DEPTH24STENCIL8
};

struct FBOTextureSpecs
{
	FBOTextureSpecs() = default;
	FBOTextureSpecs(FBOTextureFormat format)
		: TextureFormat(format)
	{
	}

	FBOTextureFormat TextureFormat = FBOTextureFormat::None;
};

struct FBOAttachmentSpecs
{
	FBOAttachmentSpecs() = default;
	FBOAttachmentSpecs(std::initializer_list<FBOTextureSpecs> attachments)
		: Attachments(attachments)
	{
	}

	std::vector<FBOTextureSpecs> Attachments;
};

struct FBOSpecs
{
	uint32_t Width = 0, Height = 0;
	FBOAttachmentSpecs Attachments;
	uint32_t Samples = 1;
	// bool SwapChainTarget = false;
};

class FrameBuffer
{
public:
	FrameBuffer(FBOSpecs const &spec);
	~FrameBuffer();

	void Bind() const;
	void Unbind() const;

	void Resize(uint32_t width, uint32_t height);
	void ClearAttachment(uint32_t attachmentIndex, int value);

	uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const;
	uint32_t GetDepthAttachmentRendererID() const { return m_DepthAttachment; }
	uint32_t GetFBOHandle() const { return m_FBOHandle; }
	const FBOSpecs &GetSpecification() const
	{
		return m_Specifications;
	}

	// MSAA support
	bool IsMultisampled() const { return m_Specifications.Samples > 1; }
	void ResolveToFramebuffer(FrameBuffer* destination) const;


private:
	void Invalidate();
	void ClearAttachments();

	uint32_t m_FBOHandle = 0;
	FBOSpecs m_Specifications;

	std::vector<FBOTextureSpecs> m_ColorAttachmentSpecs;
	FBOTextureSpecs m_DepthAttachmentSpec = FBOTextureFormat::None;

	std::vector<uint32_t> m_ColorAttachments;
	uint32_t m_DepthAttachment = 0;
};