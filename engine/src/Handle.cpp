#include "Handle.hpp"

#include "Manager/StringManager.hpp"
#include "uuid/uuid.hpp"

Handle::Handle(std::string name) : handle_name{StringManager::GetInstance().Intern_String(std::move(name))}, id{UUID<128>::Generate()}
{
}
Handle::Handle(std::string name, UUID<128> const& uuid) : handle_name{ StringManager::GetInstance().Intern_String(std::move(name)) }, id{uuid}
{
	
}
