#include <Windows.h>
#include <string>

inline std::wstring string_to_wstring(const std::string& str) {
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	std::wstring wstr(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);
	// Remove the null terminator that was included by -1 parameter
	if (!wstr.empty() && wstr.back() == L'\0') {
		wstr.pop_back();
	}
	return wstr;
}

inline std::string wstring_to_string(const std::wstring& wstr) {
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string utf8(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8[0], size_needed, nullptr, nullptr);
	// Remove the null terminator that was included by -1 parameter
	if (!utf8.empty() && utf8.back() == '\0') {
		utf8.pop_back();
	}
	return utf8;
}