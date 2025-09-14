#ifndef MONOMANAGER_HPP
#define MONOMANAGER_HPP
#include <string>
#include <vector>
#include <mono/utils/mono-forward.h>


class ScriptCompiler;
class MonoLoader;

class MonoManager {
	static ScriptCompiler* m_Compiler;
	static MonoLoader* m_Loader;
	static std::vector<std::string> m_ScriptBins;

	static bool m_Verbose;



public:
	static void Initialize();
	static void AddSearchDirectories(std::string const& path);
	static void ClearBins();

	static std::vector<std::string> const& GetBins();

	static void SetVerbose(bool v);
	static bool GetVerbose();


	static void StartCompilation();


	static ScriptCompiler* GetCompiler();
	static MonoLoader* GetLoader();
};
#endif //MONOMANAGER_HPP