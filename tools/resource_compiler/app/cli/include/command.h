#ifndef RESOURCE_COMPILER_COMMAND_H
#define RESOURCE_COMPILER_COMMAND_H

#include <unordered_map>
#include <string_view>
#include <functional>
#include <iostream>
#include <cstdint>
#include <string>
#include <memory>

// Windows headers define ERROR as a macro, which conflicts with our enum values
#ifdef ERROR
#undef ERROR
#endif

struct CommandProcessor {
	enum class Status : std::uint8_t {
		ERROR = (char)0xFF,
		STOP = (char)0x1,
		OK = (char)0x0,
	};

	enum class SEVERITY : std::uint8_t {
		ERROR = (char)0xFF,
		WARNING = (char)0x1,
		INFO = (char)0x0,
	};

	using CommandFn = std::function<void(std::string_view)>;
	using CommandPair = std::pair<std::string const, CommandFn>;

	std::unordered_map<std::string, CommandFn> _func;
	Status _s;
	std::istream& _strm{ std::cin };

	CommandProcessor(std::initializer_list<CommandPair> const& cmdp_list)
		: _func{}, _s{ Status::OK }
	{
		for (CommandPair const& cmdp : cmdp_list) {
			_func.emplace(cmdp);
		}
	}

	void Associate(CommandPair const& cmdp) {
		_func.emplace(cmdp);
	}

	CommandFn MapCommand(std::string_view cmd_key) {
		std::unordered_map<std::string, CommandFn>::iterator fn{ _func.find(std::string(cmd_key.begin(), cmd_key.end())) };
		return fn == _func.end() ? nullptr : fn->second;
	}

	void Clear() {
		_s = Status::OK;
	}
	operator bool() {
		return _s == Status::OK;
	}

	Status Process(std::string_view cmd) {
		std::string_view cmd_params{ cmd.substr(cmd.find_first_of(" \t") + 1) };
		CommandFn fn{ MapCommand(cmd.substr(0, cmd.find_first_of(" \t"))) };
		if (_s == Status::OK && !cmd.empty() && ((fn) ? _s = Status::OK : _s = Status::STOP) == Status::OK) {
			fn(cmd_params);
		}
		return _s;
	}

	Status Listen() {
		std::string cmd;
		while (*this) {
			std::getline(_strm, cmd);
			Process(cmd);
			cmd.clear();
		}
		return _s;
	}

	void Log(std::string_view msg, SEVERITY level = SEVERITY::ERROR) {
		std::cout << ((level == SEVERITY::ERROR) ? "[ERROR] " : ((level == SEVERITY::WARNING) ? "[WARNING] " : "[INFO] "));
		std::cout << msg << "\n";
	}

	static CommandProcessor& Instance() {
		if (!cp_ptr) {
			Init();
		}
		return *cp_ptr;
	}

	static void Init() {
		cp_ptr.reset(new CommandProcessor{});
	}

	static void Init(std::initializer_list<CommandPair> const& cmdp_list) {
		cp_ptr.reset(new CommandProcessor{cmdp_list});
	}

	template <typename ...args>
	static void Init(args&&...cargs) {
		cp_ptr.reset(new CommandProcessor{std::forward<args>(cargs)...});
	}

	static std::unique_ptr<CommandProcessor> cp_ptr;
};


namespace {
	auto tokenise_string_view{ [](std::string_view sv, const char delimiter) {
		std::vector<std::string_view> tokenised{};
		tokenised.reserve(8);
		for (std::uint64_t i{ 0 }, j{0}; i <= sv.size(); ++i) {
			if (i == sv.size() || sv[i] == delimiter) {
				tokenised.emplace_back(sv.substr(j, i - j));
				j = i + 1;
			}
		}
		return tokenised;
	}};
}

#endif