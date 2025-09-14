#ifndef RESOURCE_COMPILER_COMPILE_H
#define RESOURCE_COMPILER_COMPILE_H

#include <locale>
#include <codecvt>
#include <vector>
#include "command.h"
#include <compiler/compile.h>

enum compile_option_param : std::uint32_t { //stuff preceeding -Option <option params>, e.g -O "compiler/out/meshes"
	OUTPUT_DIR
};

void compile_callback(std::string_view sparam){
	CommandProcessor& cproc{ CommandProcessor::Instance()};
	std::vector<std::string_view> tokenised{tokenise_string_view(sparam, ' ')};
	std::string out_dir{Resource::get_output_dir()};
	std::uint32_t file_list_ct{};
	bool file_list_end{};
	bool is_output_directory{};
	for (std::string_view& sv : tokenised) {
		if (sv.front() == '-') {
			file_list_end = true;
			if (sv == "-O") {
				is_output_directory = true;
			}
			else if (sv == "-W") {
				//is_working_directory = true; -to do
			}
			else if (sv == "-P") {
			
			}
			else if (sv == "-Q") {
				cproc._s = CommandProcessor::Status::STOP;
			}
			else {
				cproc._s = CommandProcessor::Status::ERROR;
				cproc.Log("Unknown token! Command syntax: compress file-list -Options OPTIONAL_PARAMS");
			}
		}
		else if (!file_list_end) {
			file_list_ct++;
		}
		else if (is_output_directory) {
			if (sv.front() == '\"') {
				sv = sv.substr(1, sv.size() - 2);
			}
			Resource::set_output_dir(sv.find_first_of(":") == sv.npos? std::string(sv.begin(), sv.end()) : out_dir + std::string(sv.begin(), sv.end()));
		}
	}
	tokenised.resize(file_list_ct);

	if (cproc._s != CommandProcessor::Status::ERROR) {
		for (std::string_view& sv : tokenised) {
			if (sv.substr(sv.size() - 4) == "desc") {
				Resource::ResourceDescriptor rdesc{ Resource::ResourceDescriptor::load_descriptor(sv) };
				cproc.Log((std::string("Compiling resource ") + std::string(sv.begin(), sv.end())).c_str(), CommandProcessor::SEVERITY::INFO);
				Resource::compile_descriptor(rdesc);
				cproc.Log((std::string("Compiled resource ") + std::string(sv.begin(), sv.end())).c_str(), CommandProcessor::SEVERITY::INFO);
			}
			else {
				if (sv.front() == '\"') {
					sv = sv.substr(1, sv.size() - 2);
				}
				cproc.Log((std::string("Resource ") + std::string(sv.begin(), sv.end()) + " is not a descriptor! Auto generating descriptor data...").c_str(), CommandProcessor::SEVERITY::WARNING);
				Resource::ResourceDescriptor rdesc{ std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(std::string(sv.begin(), sv.end())) };
				cproc.Log((std::string("Compiling resource ") + std::string(sv.begin(), sv.end())).c_str(), CommandProcessor::SEVERITY::INFO);
				Resource::compile_descriptor(rdesc);
				cproc.Log((std::string("Compiled resource ") + std::string(sv.begin(), sv.end())).c_str(), CommandProcessor::SEVERITY::INFO);
			}
		}
	}
	Resource::set_output_dir(out_dir);
}

#endif