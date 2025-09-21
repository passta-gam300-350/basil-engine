#include "../../include/Pipeline/RenderPipeline.h"
#include "../../include/Pipeline/RenderContext.h"
#include <algorithm>

RenderPipeline::RenderPipeline()
{
}

RenderPipeline::~RenderPipeline()
{
	// Passes will be cleaned up by shared_ptr
}

void RenderPipeline::AddPass(const std::shared_ptr<RenderPass>& pass)
{
	m_Passes.push_back(pass);
	// Enable pass by default
	m_PassEnabled[pass->GetName()] = true;
}

void RenderPipeline::RemovePass(const std::string& name)
{
	auto it = std::remove_if(m_Passes.begin(), m_Passes.end(),
		[&name](const auto& pass) { return pass->GetName() == name; });

	if (it != m_Passes.end())
	{
		m_Passes.erase(it);
		// Remove from enabled map as well
		m_PassEnabled.erase(name);
	}
}

void RenderPipeline::Execute(RenderContext& context) const
{
	for (const auto& pass : m_Passes)
	{
		// Only execute pass if it's enabled
		if (IsPassEnabled(pass->GetName()))
		{
			pass->Execute(context);
		}
		else
		{
			// Handle disabled passes that need cleanup
			if (pass->GetName() == "ShadowPass")
			{
				// Clear shadow data when shadow pass is disabled
				context.frameData.shadowMaps.clear();
				context.frameData.shadowMatrices.clear();
			}
		}
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

void RenderPipeline::EnablePass(const std::string& name, bool enabled)
{
	// Check if pass exists before enabling/disabling
	auto passIt = std::find_if(m_Passes.begin(), m_Passes.end(),
		[&name](const auto& pass) { return pass->GetName() == name; });

	if (passIt != m_Passes.end())
	{
		m_PassEnabled[name] = enabled;
	}
}

bool RenderPipeline::IsPassEnabled(const std::string& name) const
{
	auto it = m_PassEnabled.find(name);
	if (it != m_PassEnabled.end())
	{
		return it->second;
	}
	// Default to enabled if pass not found (for backwards compatibility)
	return true;
}