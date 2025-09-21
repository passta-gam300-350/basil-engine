#ifndef FileService_HPP
#define FileService_HPP
#include <string>

#include "Service.hpp"

class FileService : public Service
{
public:
	void start() override;
	void run() override;
	void end() override;

	bool OpenFileDialog(std::string&); // OS specific file dialog
	bool OpenFolderDialog(std::string&); // OS specific folder dialog

	



};
#endif // FileService_HPP