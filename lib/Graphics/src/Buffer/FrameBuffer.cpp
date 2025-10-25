#include <Buffer/FrameBuffer.h>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <spdlog/spdlog.h>

namespace Utils
{
	static GLenum TextureFormatToGL(FBOTextureFormat format)
	{
		switch (format)
		{
		case FBOTextureFormat::RGBA8: return GL_RGBA8;
		case FBOTextureFormat::RGB8: return GL_RGB8;
		case FBOTextureFormat::SRGB8: return GL_SRGB8;
		case FBOTextureFormat::SRGB8_ALPHA8: return GL_SRGB8_ALPHA8;
		case FBOTextureFormat::RGBA16F: return GL_RGBA16F;
		case FBOTextureFormat::RGB16F: return GL_RGB16F;
		case FBOTextureFormat::RGB11F_G11F_B10F: return GL_R11F_G11F_B10F;
		case FBOTextureFormat::RED_INTEGER: return GL_R32I;
		case FBOTextureFormat::DEPTH24STENCIL8: return GL_DEPTH24_STENCIL8;
		default: return 0;
		}
	}
}

FrameBuffer::FrameBuffer(FBOSpecs const &spec)
	: m_Specifications(spec)
{
	// Check for special case: empty framebuffer (used by PresentPass which blits to screen)
	if (spec.Width == 0 && spec.Height == 0 && spec.Attachments.Attachments.empty())
	{
		spdlog::debug("Creating dummy framebuffer (0x0, no attachments) - likely for screen presentation");
		// Don't create OpenGL framebuffer objects for this special case
		m_FBOHandle = 0; // Use default framebuffer
		return;
	}

	// Seperate the color attachment specs
	for (auto format : spec.Attachments.Attachments)
	{
		if (format.TextureFormat != FBOTextureFormat::DEPTH24STENCIL8)
			m_ColorAttachmentSpecs.emplace_back(format);
		else
			m_DepthAttachmentSpec = format;
	}

	Invalidate();
}

FrameBuffer::~FrameBuffer()
{
	ClearAttachments();
	if (m_FBOHandle != 0) // Don't delete default framebuffer
	{
		glDeleteFramebuffers(1, &m_FBOHandle);
	}
}

void FrameBuffer::Invalidate()
{
	// If framebuffer already exists, we're resizing/recreating
	if (m_FBOHandle)
	{
		ClearAttachments();
		glDeleteFramebuffers(1, &m_FBOHandle);
	}

	// Ensure valid dimensions
	if (m_Specifications.Width == 0 || m_Specifications.Height == 0)
	{
		spdlog::warn("Framebuffer dimensions are invalid (Width: {}, Height: {})", m_Specifications.Width, m_Specifications.Height);
		m_Specifications.Width = 1280;
		m_Specifications.Height = 720;
	}

	// Create framebuffer
	glGenFramebuffers(1, &m_FBOHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBOHandle);

	// Create color attachments
	if (m_ColorAttachmentSpecs.size() > 0)
	{
		m_ColorAttachments.resize(m_ColorAttachmentSpecs.size());
		glGenTextures(static_cast<GLsizei>(m_ColorAttachments.size()), m_ColorAttachments.data());

		bool multisampled = IsMultisampled();
		GLenum textureTarget = multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

		for (uint32_t i = 0; i < m_ColorAttachments.size(); ++i)
		{
			glBindTexture(textureTarget, m_ColorAttachments[i]);

			// Use the actual format from specs instead of hardcoded GL_RGBA8
			GLenum internalFormat = Utils::TextureFormatToGL(m_ColorAttachmentSpecs[i].TextureFormat);

			if (multisampled)
			{
				// Create multisampled texture
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_Specifications.Samples,
					internalFormat, m_Specifications.Width, m_Specifications.Height, GL_TRUE);
			}
			else
			{
				// Determine format and type based on internal format
				GLenum format;
				GLenum type;

				switch (m_ColorAttachmentSpecs[i].TextureFormat)
				{
					case FBOTextureFormat::RGBA16F:
						format = GL_RGBA;
						type = GL_FLOAT;
						break;

					case FBOTextureFormat::RGB16F:
						format = GL_RGB;
						type = GL_FLOAT;
						break;

					case FBOTextureFormat::RGB11F_G11F_B10F:
						format = GL_RGB;
						type = GL_FLOAT;
						break;

					case FBOTextureFormat::RGB8:
					case FBOTextureFormat::SRGB8:
						format = GL_RGB;
						type = GL_UNSIGNED_BYTE;
						break;

					case FBOTextureFormat::RED_INTEGER:
						format = GL_RED_INTEGER;
						type = GL_INT;
						break;

					case FBOTextureFormat::SRGB8_ALPHA8:
					case FBOTextureFormat::RGBA8:
					default:
						format = GL_RGBA;
						type = GL_UNSIGNED_BYTE;
						break;
				}

				glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
							 m_Specifications.Width, m_Specifications.Height, 0,
							 format, type, nullptr);

				// Texture parameters (not supported for multisampled textures)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
								  textureTarget, m_ColorAttachments[i], 0);
		}

	}

	// Create depth attachment if needed
	if (m_DepthAttachmentSpec.TextureFormat != FBOTextureFormat::None)
	{
		glGenTextures(1, &m_DepthAttachment);

		bool multisampled = IsMultisampled();
		GLenum textureTarget = multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

		glBindTexture(textureTarget, m_DepthAttachment);

		if (multisampled)
		{
			// Create multisampled depth texture
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_Specifications.Samples,
				GL_DEPTH24_STENCIL8, m_Specifications.Width, m_Specifications.Height, GL_TRUE);
		}
		else
		{
			// Use standard depth-stencil format
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_Specifications.Width, m_Specifications.Height, 0,
				GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);

			// Set texture parameters for depth texture (not supported for multisampled)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			// Use border clamping for shadow maps to prevent sampling outside bounds
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

			// Set border color to white (1.0) so areas outside shadow map are not in shadow
			float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, textureTarget, m_DepthAttachment, 0);
	}

	// Specify draw buffers
	if (m_ColorAttachments.size() > 0)
	{
		GLenum buffers[16] = { 0 }; // Max color attachments is typically 16
		for (uint32_t i = 0; i < m_ColorAttachments.size(); i++)
			buffers[i] = GL_COLOR_ATTACHMENT0 + i;

		glDrawBuffers(static_cast<GLsizei>(m_ColorAttachments.size()), buffers);
	}
	else if (m_ColorAttachments.empty() && m_DepthAttachmentSpec.TextureFormat != FBOTextureFormat::None)
	{
		// Only depth-pass
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}

	// Check if framebuffer is complete
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		// Log framebuffer specifications for debugging
		spdlog::error("Framebuffer creation failed! Specs: {}x{}, ColorAttachments: {}, DepthAttachment: {}",
			m_Specifications.Width, m_Specifications.Height,
			m_ColorAttachments.size(),
			(m_DepthAttachmentSpec.TextureFormat != FBOTextureFormat::None) ? "Yes" : "No");

		switch (status)
		{
		case GL_FRAMEBUFFER_UNDEFINED:
			spdlog::error("Framebuffer is incomplete! Status: GL_FRAMEBUFFER_UNDEFINED");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			spdlog::error("Framebuffer is incomplete! Status: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			spdlog::error("Framebuffer is incomplete! Status: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
			spdlog::error("This typically means the framebuffer has no color, depth, or stencil attachments!");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			spdlog::error("Framebuffer is incomplete! Status: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			spdlog::error("Framebuffer is incomplete! Status: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER");
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			spdlog::error("Framebuffer is incomplete! Status: GL_FRAMEBUFFER_UNSUPPORTED");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			spdlog::error("Framebuffer is incomplete! Status: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			spdlog::error("Framebuffer is incomplete! Status: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS");
			break;
		default:
			spdlog::error("Framebuffer is incomplete! Status: Unknown error: 0x{:x}", status);
			break;
		}
	}
	else
	{
		spdlog::debug("Framebuffer created successfully: {}x{}, ColorAttachments: {}, DepthAttachment: {}, MSAA: {}x",
			m_Specifications.Width, m_Specifications.Height,
			m_ColorAttachments.size(),
			(m_DepthAttachmentSpec.TextureFormat != FBOTextureFormat::None) ? "Yes" : "No",
			m_Specifications.Samples);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::ClearAttachments()
{
	if (!m_ColorAttachments.empty())
	{
		glDeleteTextures(static_cast<GLsizei>(m_ColorAttachments.size()), m_ColorAttachments.data());
		m_ColorAttachments.clear();
	}

	if (m_DepthAttachment)
	{
		glDeleteTextures(1, &m_DepthAttachment);
		m_DepthAttachment = 0;
	}
}

void FrameBuffer::Bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBOHandle);
	glViewport(0, 0, m_Specifications.Width, m_Specifications.Height);
}

void FrameBuffer::Unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Restore viewport to current window size
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	// Get current window size from GLFW
	GLFWwindow* currentWindow = glfwGetCurrentContext();
	if (currentWindow) {
		int windowWidth, windowHeight;
		glfwGetFramebufferSize(currentWindow, &windowWidth, &windowHeight);
		glViewport(0, 0, windowWidth, windowHeight);
	}
}

void FrameBuffer::Resize(uint32_t width, uint32_t height)
{
	if (width == 0 || height == 0)
	{
		spdlog::error("Cannot resize framebuffer to zero dimensions!");
		return;
	}

	m_Specifications.Width = width;
	m_Specifications.Height = height;

	Invalidate();
}

void FrameBuffer::ClearAttachment(uint32_t attachmentIndex, int value)
{
	if (attachmentIndex >= m_ColorAttachments.size())
	{
		spdlog::error("Attachment index out of range!");
		return;
	}

	auto &spec = m_ColorAttachmentSpecs[attachmentIndex];
	glClearTexImage(m_ColorAttachments[attachmentIndex], 0,
		GL_RGBA, GL_INT, &value);
}

uint32_t FrameBuffer::GetColorAttachmentRendererID(uint32_t index) const
{
	if (index >= m_ColorAttachments.size())
	{
		spdlog::error("Color attachment index out of range!");
		return 0;
	}

	return m_ColorAttachments[index];
}

void FrameBuffer::ResolveToFramebuffer(FrameBuffer* destination) const
{
	if (!IsMultisampled())
	{
		spdlog::warn("Attempting to resolve non-multisampled framebuffer - use blit instead");
		return;
	}

	if (!destination)
	{
		spdlog::error("Cannot resolve to null framebuffer!");
		return;
	}

	if (destination->IsMultisampled())
	{
		spdlog::error("Cannot resolve to multisampled framebuffer - destination must be non-MSAA");
		return;
	}

	// Ensure destination size matches source
	const auto& srcSpec = GetSpecification();
	const auto& dstSpec = destination->GetSpecification();

	if (srcSpec.Width != dstSpec.Width || srcSpec.Height != dstSpec.Height)
	{
		spdlog::warn("Resolve: Source and destination sizes differ ({}x{} -> {}x{}) - will stretch",
			srcSpec.Width, srcSpec.Height, dstSpec.Width, dstSpec.Height);
	}

	// Bind framebuffers for resolve
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBOHandle);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destination->GetFBOHandle());

	// Resolve color attachments (MSAA -> non-MSAA)
	// Use GL_NEAREST for multisampled resolve (GL_LINEAR not allowed for integer formats)
	glBlitFramebuffer(
		0, 0, srcSpec.Width, srcSpec.Height,
		0, 0, dstSpec.Width, dstSpec.Height,
		GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
		GL_NEAREST
	);

	// Restore framebuffer binding
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	spdlog::debug("Resolved MSAA framebuffer ({}x samples) to non-MSAA framebuffer",
		m_Specifications.Samples);
}