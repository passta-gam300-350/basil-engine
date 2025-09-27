#include "compile.h"

std::unique_ptr<CommandProcessor> CommandProcessor::cp_ptr{};

int main(int argc, char* argv[]) {
	CommandProcessor::Init({ {"compile", compile_callback}, {"quit", [](std::string_view) {CommandProcessor::Instance()._s = CommandProcessor::Status::STOP; }} });
	CommandProcessor& cp{ CommandProcessor::Instance() };
	if (argc > 1) {
		std::string cmd;
		for (int i{1}; i < argc; i++) {
			cmd += std::string(argv[i]) + " ";
		}
		cp.Process(cmd);
	}
	cp.Listen();
	return 0;
}