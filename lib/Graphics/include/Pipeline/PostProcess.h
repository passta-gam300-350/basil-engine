#pragma once

#include "RenderPass.h"
#include "../../include/Resources/Shader.h"
#include <memory>

class PostProcessEffect
{
public:
	PostProcessEffect(const std::string& name, const std::shared_ptr<Shader>& shader);

	virtual void Apply(const std::shared_ptr<FrameBuffer>& source, std::shared_ptr<FrameBuffer>& destination);

	const std::string& GetName() const { return m_Name; }
	std::shared_ptr<Shader> GetShader() const { return m_Shader; }

protected:
	std::string m_Name;
	std::shared_ptr<Shader> m_Shader;
};

class PostProcessStack
{
public:
    PostProcessStack();
    ~PostProcessStack();

    void PushEffect(const std::shared_ptr<PostProcessEffect>& effect);
    void PopEffect();
    void ClearEffects();

    void Apply(const std::shared_ptr<FrameBuffer>& source, std::shared_ptr<FrameBuffer>& destination);

private:
    std::vector<std::shared_ptr<PostProcessEffect>> m_Effects;
    std::shared_ptr<FrameBuffer> m_IntermediateFramebuffer;
};