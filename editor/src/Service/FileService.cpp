#include "Service/Service.hpp"
#include "Service/FileService.hpp"
#ifdef _WIN32
#include <Windows.h>
#include <commdlg.h>
#include <ShlObj_core.h>
#endif
#include <filesystem>


namespace
{
#ifdef _WIN32
	std::string ConvertWStrToStr(std::wstring const& s)
	{
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, nullptr, 0, nullptr, nullptr);

		if (size_needed <= 0) return {};

		std::string n(size_needed, '\0'); // allocate buffer
		WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, &n[0], size_needed, nullptr, nullptr);
		n.resize(size_needed - 1); // remove the null terminator added by WideCharToMultiByte

		return n;
	}
#endif
}

void FileService::start()
{
	if (state == ServiceState::STOPPED)
	{
		state = ServiceState::STARTING;
#ifdef _WIN32
		
#endif
		state = ServiceState::RUNNING;
	}
}
void FileService::run()
{
	if (state != ServiceState::RUNNING) return;
#ifdef _WIN32
#endif
}

void FileService::end()
{
	if (state != ServiceState::RUNNING) return;
	state = ServiceState::ENDING;
#ifdef _WIN32

#endif
	state = ServiceState::STOPPED;
	
}

bool FileService::OpenFileDialog(std::string& input_path)
{
	input_path.clear();
#ifdef _WIN32
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr))
		return false;

	IFileDialog* pFileDialog = nullptr;
	hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pFileDialog));
	if (SUCCEEDED(hr))
	{
		DWORD options = 0;
		if (SUCCEEDED(pFileDialog->GetOptions(&options)))
		{
			pFileDialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST);
		}
		const COMDLG_FILTERSPEC fileTypes[] =
		{
			{ L"All Files", L"*.*" }
		};
		pFileDialog->SetFileTypes(ARRAYSIZE(fileTypes), fileTypes);
		hr = pFileDialog->Show(nullptr);
		if (SUCCEEDED(hr))
		{
			IShellItem* pItem = nullptr;
			if (SUCCEEDED(pFileDialog->GetResult(&pItem)))
			{
				PWSTR path = nullptr;
				if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &path)))
				{
					std::wstring wpath(path);
					input_path = ConvertWStrToStr(wpath);
					CoTaskMemFree(path);
				}
				pItem->Release();
				pFileDialog->Release();
				CoUninitialize();
				return true;
			}
		}
		pFileDialog->Release();
	}

	CoUninitialize();
	return false;
#else
	input_path = "NOT IMPLEMENTED FOR THIS OS";
	return false;
#endif
}



bool FileService::OpenFolderDialog(std::string& output_path)
{
	output_path.clear();
#ifdef _WIN32
	IFileDialog* pFileDialog = nullptr;

	if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr,
		CLSCTX_ALL, IID_PPV_ARGS(&pFileDialog))))
	{
		// Options: folder selection only, allow modern UI
		DWORD options;
		if (SUCCEEDED(pFileDialog->GetOptions(&options)))
		{
			pFileDialog->SetOptions(options | FOS_PICKFOLDERS);
		}

		// Show dialog
		if (SUCCEEDED(pFileDialog->Show(nullptr)))
		{
			IShellItem* pItem = nullptr;
			if (SUCCEEDED(pFileDialog->GetResult(&pItem)))
			{
				PWSTR path = nullptr;
				if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &path)))
				{
					std::wstring op =  std::wstring(path, path + wcslen(path));
					// Convert to UTF-8
					output_path = ConvertWStrToStr(op);
					CoTaskMemFree(path);
				}
				pItem->Release();
				pFileDialog->Release();
				return true;
			}
		}
		pFileDialog->Release();
	}

	return false;
#else
	output_path = "NOT IMPLEMENTED FOR THIS OS";
	return false;
#endif
}
