#pragma once

#include <string>
#include <vector>
#include <deque>
#include <optional>
#include <mutex>
#include <chrono>

enum class ErrorCode
{
	Init_ConfigLoadFailed,
	Init_WindowCreationFailed,
	Init_SubsystemFailed,
	Init_SceneLoadFailed,
	Runtime_UnhandledException,
	Runtime_UnknownException
};

struct ErrorEvent
{
	ErrorCode code{ErrorCode::Runtime_UnknownException};
	std::string message;
	std::string stacktrace;
	double timestamp{0.0};
};

std::string CaptureCurrentStacktrace(unsigned skipFrames = 0);
const char* ErrorCodeToString(ErrorCode code);

class ErrorQueue
{
public:
	static ErrorQueue& Instance();

	void Push(ErrorEvent event);
	std::optional<ErrorEvent> Pop();
	std::vector<ErrorEvent> Drain();
	bool HasErrors() const;
	void Clear();

private:
	ErrorQueue() = default;
	mutable std::mutex m_Mutex;
	std::deque<ErrorEvent> m_Queue;
};
