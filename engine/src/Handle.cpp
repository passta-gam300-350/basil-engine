#include "Handle.hpp"
#include "uuid/uuid.hpp"

Handle::Handle(std::string const& name) : handle_name{name}, id{UUID<128>::Generate()}
{
}
Handle::Handle(std::string const& name, UUID<128> const& uuid) : handle_name{name}, id{uuid}
{
	
}
