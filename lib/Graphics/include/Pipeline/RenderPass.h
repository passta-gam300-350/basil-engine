/******************************************************************************/
/*!
\file   RenderPass.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the base RenderPass class for rendering pipeline stages

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "../Buffer/FrameBuffer.h"
#include "../Core/RenderCommandBuffer.h"
#include "../Utility/Viewport.h"
#include <memory>
#include <string>

// Forward declaration
struct RenderContext;

/**
 * @brief Defines how a render pass handles window resize events
 */
enum class ResizeMode {
	Fixed,          // Fixed-size framebuffer (e.g., shadow maps) - never resizes
	MatchViewport,  // Auto-resize framebuffer to match viewport dimensions
	Custom          // Custom resize logic implemented via OnResize() callback
};

class RenderPass
{
public:
	// Constructor for passes with framebuffers
	RenderPass(const std::string& name, const FBOSpecs& spec);
	// Constructor for passes without framebuffers (like debug passes)
	RenderPass(const std::string& name);
	virtual ~RenderPass() = default;

	virtual void Begin();
	virtual void Execute(RenderContext& context) = 0;
	virtual void End();

	// Setup the command buffer with required systems from context
	void SetupCommandBuffer(RenderContext& context);

	std::shared_ptr<FrameBuffer> GetFramebuffer() const { return m_Framebuffer; }
	const std::string& GetName() const { return m_Name; }
	bool HasFramebuffer() const { return m_Framebuffer != nullptr; }

	// Viewport management
	void SetViewport(const Viewport& viewport) { m_Viewport = viewport; }
	const Viewport& GetViewport() const { return m_Viewport; }

	// Pass-isolated command buffer API
	void Submit(const VariantRenderCommand& command);
	void ExecuteCommands() const;
	void ClearCommands();

	// Resize handling API
	void SetResizeMode(ResizeMode mode) { m_ResizeMode = mode; }
	ResizeMode GetResizeMode() const { return m_ResizeMode; }

protected:
	std::string m_Name;
	std::shared_ptr<FrameBuffer> m_Framebuffer; // Optional - can be nullptr
	Viewport m_Viewport;

	// Pass-isolated command buffer for state isolation
	std::unique_ptr<RenderCommandBuffer> m_PassCommandBuffer;

	// Resize handling
	ResizeMode m_ResizeMode = ResizeMode::Fixed;
	uint32_t m_LastViewportWidth = 0;
	uint32_t m_LastViewportHeight = 0;

	// Helper: Call this at the start of Execute() to auto-handle resize
	void CheckAndResizeIfNeeded(const RenderContext& context);

	// Override this for custom resize logic (when ResizeMode::Custom)
	virtual void OnResize(uint32_t newWidth, uint32_t newHeight);
};