#include "../../include/Pipeline/RenderPipeline.h"
#include <algorithm>

RenderPipeline::RenderPipeline(const std::string& name)
	: m_Name(name)
{
}

RenderPipeline::~RenderPipeline()
{
	// Passes will be cleaned up by shared_ptr
}

RenderPipeline::AddPass(const std::shared_ptr<RenderPass>& pass)
{
	m_Passes.push_back(pass);
}

void RenderPipeline::RemovePass(const std::string& name)
{
	auto it = std::remove_if(m_Passes.begin(), m_Passes.end(),
		[&name](const auto& pass) { return pass->GetName() == name; });
	
	if (it != m_Passes.end())
	{
		m_Passes.erase(it);
	}
}

void RenderPipeline::Execute()
{
	for (const auto& pass : m_Passes)
	{
		pass->Execute();
	}
}

std::shared_ptr<RenderPass> RenderPipeline::GetPass(const std::string& name)
{
	auto it = std::find_if(m_Passes.begin(), m_Passes.end(),
		[&name](const auto& pass) { return pass->GetName() == name; });
	
	if (it != m_Passes.end())
	{
		return *it;
	}
	
	return nullptr; // Pass not found
}