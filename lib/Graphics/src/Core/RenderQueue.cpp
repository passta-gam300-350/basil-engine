#include "../../include/Core/RenderQueue.h"

RenderQueue::RenderQueue()
{
	m_Commands.reserve(100); // Reserve space for 100 commands to avoid frequent reallocations
}

RenderQueue::~RenderQueue()
{
	Clear(); // Clear commands to ensure proper cleanup
}

void RenderQueue::Submit(const RenderCommand& command)
{
	// Clone the command and add it to the queue
	//std::cout << "RenderQueue: Command submitted, queue size now: " << (m_Commands.size() + 1) << std::endl;
	m_Commands.emplace_back(std::unique_ptr<RenderCommand>(command.Clone()));
}

void RenderQueue::Execute()
{
	// Execute all commands in the queue
	//std::cout << "RenderQueue: Executing " << m_Commands.size() << " commands" << std::endl;
	for (const auto& command : m_Commands)
	{
		command->Execute();
	}
}

void RenderQueue::Clear()
{
	// Clear the command queue
	m_Commands.clear();
}