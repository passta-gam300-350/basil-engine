#include <Buffer/FrameBuffer.h>
#include <glad/gl.h>
#include <iostream>

namespace Utils
{
	static GLenum TextureFormatToGL(FBOTextureFormat format)
	{
		switch (format)
		{
		case FBOTextureFormat::RGBA8: return GL_RGBA8;
		case FBOTextureFormat::RGBA16F: return GL_RGBA16F;
		case FBOTextureFormat::RED_INTEGER: return GL_R32I;
		case FBOTextureFormat::DEPTH24STENCIL8: return GL_DEPTH24_STENCIL8;
		default: return 0;
		}
	}
}

FrameBuffer::FrameBuffer(FBOSpecs const &spec)
	: m_Specifications(spec)
{
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
	glDeleteFramebuffers(1, &m_FBOHandle);
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
		std::cerr << "Warning: Framebuffer dimensions are invalid (Width: " << m_Specifications.Width 
				  << ", Height: " << m_Specifications.Height << ")" << std::endl;
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

		for (uint32_t i = 0; i < m_ColorAttachments.size(); ++i)
		{
			glBindTexture(GL_TEXTURE_2D, m_ColorAttachments[i]);
			
			// Use standard RGBA8 format for color attachments
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Specifications.Width, m_Specifications.Height, 0,
				GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_ColorAttachments[i], 0);
		}

	}

	// Create depth attachment if needed
	if (m_DepthAttachmentSpec.TextureFormat != FBOTextureFormat::None)
	{
		glGenTextures(1, &m_DepthAttachment);
		glBindTexture(GL_TEXTURE_2D, m_DepthAttachment);

		// Use standard depth-stencil format
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_Specifications.Width, m_Specifications.Height, 0,
			GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
		
		// Set texture parameters for depth texture
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachment, 0);
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
		std::cerr << "Framebuffer is incomplete! Status: ";
		switch (status)
		{
		case GL_FRAMEBUFFER_UNDEFINED:
			std::cerr << "GL_FRAMEBUFFER_UNDEFINED" << std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT" << std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT" << std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER" << std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER" << std::endl;
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			std::cerr << "GL_FRAMEBUFFER_UNSUPPORTED" << std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE" << std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS" << std::endl;
			break;
		default:
			std::cerr << "Unknown error: 0x" << std::hex << status << std::endl;
			break;
		}
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
}

void FrameBuffer::Resize(uint32_t width, uint32_t height)
{
	if (width == 0 || height == 0)
	{
		std::cerr << "Cannot resize framebuffer to zero dimensions!" << std::endl;
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
		std::cerr << "Attachment index out of range!" << std::endl;
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
		std::cerr << "Color attachment index out of range!" << std::endl;
		return 0;
	}

	return m_ColorAttachments[index];
}