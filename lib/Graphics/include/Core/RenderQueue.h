#pragma once

#include <iostream>
#include "RenderCommand.h"
#include <vector>
#include <memory>

class RenderQueue
{
public:
	RenderQueue();
	~RenderQueue();

	void Submit(const RenderCommand& command);
	void Execute();
	void Clear();

	size_t Size() const { return m_Commands.size(); }

private:
	std::vector<std::unique_ptr<RenderCommand>> m_Commands;
};