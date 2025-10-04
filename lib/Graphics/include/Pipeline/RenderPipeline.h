/******************************************************************************/
/*!
\file   RenderPipeline.h
\author Team PASSTA
        Bryan Ang Wei Ze (bryanweize.ang@digipen.edu)
        Tham Kang Ting (kangting.t@digipen.edu)
        Cheong Jia Zen (jiazen.c@digipen.edu)
\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief    Declares the RenderPipeline class for managing multiple render passes

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#pragma once

#include "RenderPass.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

// Forward declaration
struct RenderContext;

class RenderPipeline
{
public:
	RenderPipeline();
	RenderPipeline(const RenderPipeline&) = delete;
	RenderPipeline& operator=(const RenderPipeline&) = delete;
	RenderPipeline(RenderPipeline&&) = delete;
	RenderPipeline& operator=(RenderPipeline&&) = delete;
	~RenderPipeline();

	void AddPass(const std::shared_ptr<RenderPass>& pass);
	void RemovePass(const std::string& name);
	void Execute(RenderContext& context) const;

	std::shared_ptr<RenderPass> GetPass(const std::string& name);

	// Pass-level enable/disable control
	void EnablePass(const std::string& name, bool enabled = true);
	bool IsPassEnabled(const std::string& name) const;

private:
	std::vector<std::shared_ptr<RenderPass>> m_Passes;
	std::unordered_map<std::string, bool> m_PassEnabled;
};