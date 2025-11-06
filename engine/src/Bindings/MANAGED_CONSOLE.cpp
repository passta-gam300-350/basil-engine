#include "Bindings/MANAGED_CONSOLE.hpp"
#include <mono/metadata/object.h>
#include <span>
#include <ranges>


std::vector<std::pair<int, ManagedConsole::Message>> ManagedConsole::s_Messages{};
std::mutex ManagedConsole::s_MessagesMutex{};
void ManagedConsole::Initialize() {
	s_Messages.clear();
}
void ManagedConsole::Shutdown() {
	s_Messages.clear();
}
void ManagedConsole::LogInfo(MonoString* mono_message) {
	
	auto message = mono_string_to_utf8(mono_message);


	auto it = std::find_if(s_Messages.begin(), s_Messages.end(), [&message](const std::pair<int, Message>& msg) {
		return msg.second.type == Message::Type::Info && msg.second.content == message;
	});
	if (it == s_Messages.end()) {
		std::lock_guard<std::mutex> lock(s_MessagesMutex);
		s_Messages.push_back({ 1, { Message::Type::Info, message } });
	}
	else {
		it->first += 1; // Increment count
	}
	mono_free(message);
}
void ManagedConsole::LogWarning(MonoString* mono_message) {
	auto message = mono_string_to_utf8(mono_message);
	auto it = std::find_if(s_Messages.begin(), s_Messages.end(), [&message](const std::pair<int, Message>& msg) {
		return msg.second.type == Message::Type::Warning && msg.second.content == message;
	});
	if (it == s_Messages.end()) {
		std::lock_guard<std::mutex> lock(s_MessagesMutex);
		s_Messages.push_back({ 1, { Message::Type::Warning, message } });
	}
	else {
		it->first += 1; // Increment count
	}
	mono_free(message);
}
void ManagedConsole::LogError(MonoString* mono_message) {
	auto message = mono_string_to_utf8(mono_message);
	auto it = std::find_if(s_Messages.begin(), s_Messages.end(), [&message](const std::pair<int, Message>& msg) {
		return msg.second.type == Message::Type::Error && msg.second.content == message;
	});
	if (it == s_Messages.end()) {
		std::lock_guard<std::mutex> lock(s_MessagesMutex);
		s_Messages.push_back({ 1, { Message::Type::Error, message } });
	}
	else {
		it->first += 1; // Increment count
	}

	mono_free(message);
}


std::vector<std::pair<int, ManagedConsole::Message>> ManagedConsole::TryGetMessages(unsigned filters) {
	if (s_MessagesMutex.try_lock()) {
		// Successfully acquired the lock
		auto filteredMessages = s_Messages | std::views::filter([filters](const std::pair<int, Message>& msg) {
			switch (msg.second.type) {
			case Message::Type::Info:
				return (filters & 0x1) != 0;
			case Message::Type::Warning:
				return (filters & 0x2) != 0;
			case Message::Type::Error:
				return (filters & 0x4) != 0;
			default:
				return false;
			}
		});
		auto data = std::vector<std::pair<int, Message>>(filteredMessages.begin(), filteredMessages.end());
		s_MessagesMutex.unlock();
		return data;
		
	}
	else return {}; // Failed to acquire the lock
}