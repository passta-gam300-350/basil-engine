/******************************************************************************/
/*!
\file   ENGINE_CONSOLE.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2025/11/27
\brief This file contains the implementation for the EngineConsole class, which
is responsible for handling logging messages from the engine (spdlog).


Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#ifdef NDEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#endif

#include "Console/ENGINE_CONSOLE.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>
#include <algorithm>

// Static members
std::vector<EngineConsole::Message> EngineConsole::s_EngineMessages{};
std::mutex EngineConsole::s_MessagesMutex{};

void EngineConsole::Initialize() {
	std::lock_guard<std::mutex> lock(s_MessagesMutex);
	s_EngineMessages.clear();
}

void EngineConsole::Shutdown() {
	std::lock_guard<std::mutex> lock(s_MessagesMutex);
	s_EngineMessages.clear();
}

void EngineConsole::Log(Message::Type type, const std::string& message) {
	std::lock_guard<std::mutex> lock(s_MessagesMutex);
	// Simple append - no deduplication on engine side (fast!)
	s_EngineMessages.push_back({ type, message });
}

std::vector<EngineConsole::Message> EngineConsole::TransferMessages() {
	std::lock_guard<std::mutex> lock(s_MessagesMutex);
	// Move all messages out and clear engine buffer (short critical section!)
	std::vector<Message> transferred = std::move(s_EngineMessages);
	s_EngineMessages.clear();
	return transferred;
}

// ========================================
// Custom spdlog sink
// ========================================
namespace {
	// Custom spdlog sink that forwards messages to EngineConsole
	template<typename Mutex>
	class EngineSink : public spdlog::sinks::base_sink<Mutex> {
	protected:
		void sink_it_(const spdlog::details::log_msg& msg) override {
			// Format the message
			spdlog::memory_buf_t formatted;
			this->formatter_->format(msg, formatted);
			std::string message = fmt::to_string(formatted);

			// Remove trailing newline if present
			if (!message.empty() && message.back() == '\n') {
				message.pop_back();
			}

			// Route to appropriate log level
			EngineConsole::Message::Type type;
			switch (msg.level) {
			case spdlog::level::trace:
				type = EngineConsole::Message::Type::Trace;
				break;
			case spdlog::level::debug:
				type = EngineConsole::Message::Type::Debug;
				break;
			case spdlog::level::info:
				type = EngineConsole::Message::Type::Info;
				break;
			case spdlog::level::warn:
				type = EngineConsole::Message::Type::Warn;
				break;
			case spdlog::level::err:
				type = EngineConsole::Message::Type::Error;
				break;
			case spdlog::level::critical:
				type = EngineConsole::Message::Type::Critical;
				break;
			default:
				type = EngineConsole::Message::Type::Undefined;
				break;
			}

			EngineConsole::Log(type, message);
		}

		void flush_() override {
			// Nothing to flush for our in-memory console
		}
	};

	using EngineSink_mt = EngineSink<std::mutex>;
}

// Initialize the engine console sink
static bool s_EngineConsoleSinkInitialized = false;
void InitializeEngineConsoleSink() {
	if (!s_EngineConsoleSinkInitialized) {
		EngineConsole::Initialize();

		// Create and add the sink to the default logger
		auto sink = std::make_shared<EngineSink_mt>();
		sink->set_level(spdlog::level::trace); // Capture all log levels

		// Add to default logger
		auto logger = spdlog::default_logger();
		logger->sinks().push_back(sink);

		s_EngineConsoleSinkInitialized = true;
	}
}
