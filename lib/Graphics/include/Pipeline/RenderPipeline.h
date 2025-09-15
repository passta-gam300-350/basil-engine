#pragma once

#include "RenderPass.h"
#include <memory>
#include <string>
#include <vector>

class RenderPipeline
{
public:
	RenderPipeline(const std::string& name = "DefaultPipeline");
	~RenderPipeline();

	void AddPass(const std::shared_ptr<RenderPass>& pass);
	void RemovePass(const std::string& name);
	virtual void Execute();

	std::shared_ptr<RenderPass> GetPass(const std::string& name);
	const std::string& GetName() const { return m_Name; }

private:
	std::vector<std::shared_ptr<RenderPass>> m_Passes;
	std::string m_Name;
};