/******************************************************************************/
/*!
\file   PostProcess.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the PostProcessEffect and PostProcessStack classes for post-processing effects

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
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