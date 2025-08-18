#include "../../include/Pipeline/PostProcess.h"
#include <glad/gl.h>

PostProcessEffect::PostProcessEffect(const std::string& name, const std::shared_ptr<Shader>& shader)
	: m_Name(name), m_shader(shader)
{
}

void PostProcessEffect::Apply(const std::shared_ptr<FrameBuffer>& source, std::shared_ptr<FrameBuffer>& destination)
{
	// Bind the input framebuffer as the source texture
	destination->Bind();

	// Clear destination framebuffer
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Bind the shader program
	m_Shader->Bind();
	
    // Bind source texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, source->GetColorAttachmentRendererID());
    m_Shader->SetInt("u_SourceTexture", 0);

    // Draw fullscreen quad
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_DEPTH_TEST);

	// Unbind the shader
	m_Shader->Unbind();

	// Unbind the destination framebuffer
	destination->Unbind();
}

// PostProcessStack implementation
PostProcessStack::PostProcessStack()
{
	// Create an intermediate framebuffer for ping-pong rendering
    FBOSpecs spec;
    spec.Width = 1280; // Default size, should be resized to match the screen
    spec.Height = 720;
    spec.Attachments = { { FBOTextureFormat::RGBA8 } };

    m_IntermediateFramebuffer = std::make_shared<FrameBuffer>(spec);
}

PostProcessStack::~PostProcessStack()
{
    // Effects and framebuffer will be cleaned up by shared_ptr
}

void PostProcessStack::PushEffect(const std::shared_ptr<PostProcessEffect>& effect)
{
    m_Effects.push_back(effect);
}

void PostProcessStack::PopEffect()
{
    if (!m_Effects.empty())
    {
        m_Effects.pop_back();
    }
}

void PostProcessStack::ClearEffects()
{
    m_Effects.clear();
}

void PostProcessStack::Apply(const std::shared_ptr<FrameBuffer>& source, std::shared_ptr<FrameBuffer>& destination)
{
    if (m_Effects.empty())
    {
        // No effects, just copy source to destination
        // This would typically be done with a simple blit
        auto sourceSpec = source->GetSpecification();
        auto destSpec = destination->GetSpecification();

        glBindFramebuffer(GL_READ_FRAMEBUFFER, source->GetColorAttachmentRendererID());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destination->GetColorAttachmentRendererID());
        glBlitFramebuffer(0, 0, sourceSpec.Width, sourceSpec.Height,
            0, 0, destSpec.Width, destSpec.Height,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
        return;
    }

    // Ping-pong between source and intermediate buffer for all effects except the last one
    std::shared_ptr<FrameBuffer> currentSource = source;
    std::shared_ptr<FrameBuffer> currentDest = m_IntermediateFramebuffer;

    for (size_t i = 0; i < m_Effects.size(); ++i)
    {
        // For the last effect, output directly to destination
        if (i == m_Effects.size() - 1)
        {
            currentDest = destination;
        }

        // Apply the effect
        m_Effects[i]->Apply(currentSource, currentDest);

        // Swap buffers for the next effect
        currentSource = currentDest;
        currentDest = (currentSource == m_IntermediateFramebuffer) ? source : m_IntermediateFramebuffer;
    }
}