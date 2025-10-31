/******************************************************************************/
/*!
\file   FileService.hpp
\author Team PASSTA
		Yeo Jia Hao (jiahao.yeo\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief This file contains the declaration for the FileService class, which
provides file and folder dialog functionalities for opening and saving files.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#ifndef FileService_HPP
#define FileService_HPP
#include <string>
#include <vector>

#include "Service.hpp"

class FileService : public Service
{
public:
#ifdef WIN32
	using FILE_TYPE = std::pair<const wchar_t*, const wchar_t*>;
	using FILE_TYPE_LIST = std::vector<FILE_TYPE>;
#else
	using FILE_TYPE = std::pair<const wchar_t*, const wchar_t*>;
	using FILE_TYPE_LIST = std::vector<FILE_TYPE>;
#endif 
	void start() override;
	void run() override;
	void end() override;

	bool OpenFileDialog(std::string&); // OS specific file dialog
	bool OpenFileDialog(const char* defaultPath, std::string&, FILE_TYPE_LIST const& fileTypes = {}); // OS specific file dialog with default path
	bool SaveFileDialog(const char* defaultPath, std::string&, FILE_TYPE_LIST const& fileTypes = {}); // OS specific file dialog with default path

	bool OpenFolderDialog(std::string&); // OS specific folder dialog

	



};
#endif // FileService_HPP