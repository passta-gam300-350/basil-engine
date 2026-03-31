#include "Diagnostics/ErrorQueue.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#else
#include <execinfo.h>
#endif

#include <sstream>
#include <cstdio>

const char* ErrorCodeToString(ErrorCode code)
{
	switch (code)
	{
	case ErrorCode::Init_ConfigLoadFailed:       return "Config Load Failed";
	case ErrorCode::Init_WindowCreationFailed:   return "Window Creation Failed";
	case ErrorCode::Init_SubsystemFailed:         return "Subsystem Init Failed";
	case ErrorCode::Init_SceneLoadFailed:         return "Scene Load Failed";
	case ErrorCode::Runtime_UnhandledException:   return "Unhandled Exception";
	case ErrorCode::Runtime_UnknownException:     return "Unknown Exception";
	default:                                      return "Unknown Error";
	}
}

#ifdef _WIN32

class SymbolInitializer
{
public:
	SymbolInitializer()
	{
		SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
		m_Initialized = SymInitialize(GetCurrentProcess(), NULL, TRUE);
	}

	~SymbolInitializer()
	{
		if (m_Initialized)
			SymCleanup(GetCurrentProcess());
	}

	bool IsInitialized() const { return m_Initialized; }

private:
	bool m_Initialized = false;
};

static SymbolInitializer& GetSymbolInitializer()
{
	static SymbolInitializer sinit;
	return sinit;
}

std::string CaptureCurrentStacktrace(unsigned skipFrames)
{
	GetSymbolInitializer();

	constexpr unsigned kMaxFrames = 64;
	void* frames[kMaxFrames];

 USHORT captured = CaptureStackBackTrace(
  static_cast<DWORD>(1 + skipFrames),
  kMaxFrames,
  frames,
  nullptr);

	if (captured == 0)
		return "  (no stacktrace available)\n";

 SYMBOL_INFO_PACKAGE sip{};
	sip.si.MaxNameLen = 512;
	sip.si.SizeOfStruct = sizeof(SYMBOL_INFO);

	IMAGEHLP_LINE64 line{};
	line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	DWORD displacement = 0;

	std::ostringstream oss;

	for (USHORT i = 0; i < captured; ++i)
	{
		DWORD64 addr = reinterpret_cast<DWORD64>(frames[i]);

		BOOL hasSym = SymFromAddr(GetCurrentProcess(), addr, nullptr, &sip.si);
		BOOL hasLine = SymGetLineFromAddr64(GetCurrentProcess(), addr, &displacement, &line);

		char buf[1024];
		if (hasSym && hasLine)
		{
			std::snprintf(buf, sizeof(buf), "  [%2u] %s (%s:%lu)\n",
				i, sip.si.Name, line.FileName, line.LineNumber);
		}
		else if (hasSym)
		{
			std::snprintf(buf, sizeof(buf), "  [%2u] %s (0x%016llX)\n",
				i, sip.si.Name, static_cast<unsigned long long>(addr));
		}
		else
		{
			std::snprintf(buf, sizeof(buf), "  [%2u] 0x%016llX\n",
				i, static_cast<unsigned long long>(addr));
		}
		oss << buf;
	}

	return oss.str();
}

#else

std::string CaptureCurrentStacktrace(unsigned skipFrames)
{
	constexpr int kMaxFrames = 64;
	void* buffer[kMaxFrames];

	int captured = backtrace(buffer, kMaxFrames);
	if (captured <= static_cast<int>(skipFrames))
		return "  (no stacktrace available)\n";

	char** symbols = backtrace_symbols(buffer, captured);
	if (!symbols)
		return "  (no stacktrace available)\n";

	std::ostringstream oss;
	for (int i = static_cast<int>(skipFrames); i < captured; ++i)
	{
		oss << "  [" << (i - static_cast<int>(skipFrames)) << "] "
		    << (symbols[i] ? symbols[i] : "???") << "\n";
	}
	free(symbols);
	return oss.str();
}

#endif

ErrorQueue& ErrorQueue::Instance()
{
	static ErrorQueue s_Instance;
	return s_Instance;
}

void ErrorQueue::Push(ErrorEvent event)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Queue.push_back(std::move(event));
}

std::optional<ErrorEvent> ErrorQueue::Pop()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (m_Queue.empty())
		return std::nullopt;
	ErrorEvent evt = std::move(m_Queue.front());
	m_Queue.pop_front();
	return evt;
}

std::vector<ErrorEvent> ErrorQueue::Drain()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	std::vector<ErrorEvent> result;
	result.reserve(m_Queue.size());
	for (auto& evt : m_Queue)
		result.push_back(std::move(evt));
	m_Queue.clear();
	return result;
}

bool ErrorQueue::HasErrors() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return !m_Queue.empty();
}

void ErrorQueue::Clear()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_Queue.clear();
}
