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