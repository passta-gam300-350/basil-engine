/******************************************************************************/
/*!
\file   ENGINE_CONSOLE.hpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2025/11/27
\brief This file contains the declaration for the EngineConsole class, which
is responsible for handling logging messages from the engine (spdlog).


Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/

#ifndef ENGINE_CONSOLE_HPP
#define ENGINE_CONSOLE_HPP

#include <string>
#include <vector>
#include <mutex>

class EngineConsole {
public:
	struct Message {
		enum class Type {
			Trace,
			Debug,
			Info,
			Warn,
			Error,
			Critical,
			Undefined
		} type;

		std::string content;
	};

private:
	static std::vector<Message> s_EngineMessages;  // Engine-side buffer (no counts, just messages)
	static std::mutex s_MessagesMutex;

public:
	static void Initialize();
	static void Shutdown();

	static void Log(Message::Type type, const std::string& message);

	// Transfer messages from engine buffer to caller, clearing engine buffer
	static std::vector<Message> TransferMessages();
};

// Initialize the spdlog sink (call once at startup)
void InitializeEngineConsoleSink();

#endif // ENGINE_CONSOLE_HPP
