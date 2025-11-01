#ifndef StringManager_HPP
#define StringManager_HPP
#include <cstdint>
#include <string>
#include <unordered_map>
class StringManager
{
public:
	static StringManager* _instance;
	using StringID = uint32_t;
	using offset_t = uint32_t;
private:
	std::string container{};
	std::unordered_map<StringID, offset_t> StringTable;

	uint32_t HashString(std::string const& name);
public:
	

	static StringManager& GetInstance();
	StringID Intern_String(std::string const&& str);
	std::string_view Get_String(StringID id);
	void Clear();
	
};

#endif