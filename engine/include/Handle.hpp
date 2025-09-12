#ifndef HANDLE_HPP
#define HANDLE_HPP
#include <string>

#include "uuid/uuid.hpp"


struct Handle
{
	std::string handle_name;
	UUID<128> id;
	Handle() = default;
	Handle(std::string const& name);
	Handle(std::string const& name, UUID<128> const& uuid);
};

#endif // !HANDLE_HPP