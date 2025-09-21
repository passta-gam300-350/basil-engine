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