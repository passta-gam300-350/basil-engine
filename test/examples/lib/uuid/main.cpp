#include <iostream>

#include "uuid/uuid.hpp"
int main(int argc, char* argv[]) {
	UUID<128> uuid128 = UUID<128>::Generate();
	std::cout << "Generated UUID (128 bits): " << uuid128.ToString() << '\n';
	UUID<64> uuid64 = UUID<64>::Generate();
	std::cout << "Generated UUID (64 bits): " << uuid64.ToString() << '\n';
}
