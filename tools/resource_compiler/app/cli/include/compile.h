#ifndef RESOURCE_COMPILER_COMPILE_H
#define RESOURCE_COMPILER_COMPILE_H

#include <locale>
#include <codecvt>
#include <vector>
#include "command.h"
#include <importer/importer.hpp>
#include <filesystem>

enum compile_option_param : std::uint32_t { //stuff preceeding -Option <option params>, e.g -O "compiler/out/meshes"
	OUTPUT_DIR
};

std::string OutputDir{std::filesystem::current_path().string()};

void compile_callback(std::string_view sparam){
	CommandProcessor& cproc{ CommandProcessor::Instance()};
	std::vector<std::string_view> tokenised{tokenise_string_view(sparam, ' ')};
	std::string out_dir{ OutputDir };
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
			OutputDir = (sv.find_first_of(":") == sv.npos? std::string(sv.begin(), sv.end()) : out_dir + std::string(sv.begin(), sv.end()));
		}
	}
	tokenised.resize(file_list_ct);

	if (cproc._s != CommandProcessor::Status::ERROR) {
		for (std::string_view& sv : tokenised) {
			if (sv.substr(sv.size() - 4) == "desc") {
				Resource::ResourceDescriptor rdesc{ Resource::ResourceDescriptor::LoadDescriptor(sv) };
				cproc.Log((std::string("Compiling resource ") + std::string(sv.begin(), sv.end())).c_str(), CommandProcessor::SEVERITY::INFO);
				Resource::Import(rdesc);
				cproc.Log((std::string("Compiled resource ") + std::string(sv.begin(), sv.end())).c_str(), CommandProcessor::SEVERITY::INFO);
			}
			else {
				cproc.Log((std::string("File is not a descriptor! file: ") + std::string(sv.begin(), sv.end())).c_str(), CommandProcessor::SEVERITY::ERROR);
			}
		}
	}
	OutputDir = out_dir;
}

#endif