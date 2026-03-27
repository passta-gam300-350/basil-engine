/******************************************************************************/
/*!
\file   main.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  UUID test driver

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include <iostream>

#include "uuid/uuid.hpp"
using namespace uuid;
int main(int argc, char* argv[]) {
	UUID<128> uuid128 = UUID<128>::Generate();
	std::cout << "Generated UUID (128 bits): " << uuid128.ToString() << '\n';
	UUID<64> uuid64 = UUID<64>::Generate();
	std::cout << "Generated UUID (64 bits): " << uuid64.ToString() << '\n';
}
