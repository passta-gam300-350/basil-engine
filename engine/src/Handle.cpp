#include "Handle.hpp"

#include "Manager/StringManager.hpp"
#include "uuid/uuid.hpp"

ComponentHandle::ComponentHandle(std::string name) : handle_name{StringManager::GetInstance().Intern_String(std::move(name))}, id{uuid::UUID<128>::Generate()}
{
}
ComponentHandle::ComponentHandle(std::string name, uuid::UUID<128> const& uuid) : handle_name{ StringManager::GetInstance().Intern_String(std::move(name)) }, id{uuid}
{

}
