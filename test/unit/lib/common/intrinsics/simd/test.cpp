/******************************************************************************/
/*!
\file   test.cpp
\author Team PASSTA
\par    Course : CSD3401 / UXG3400
\date   2026/01/16
\brief  SIMD intrinsics unit tests

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
/******************************************************************************/
#include <simd.h>
#include <iostream>
#include <glm/glm.hpp>
#include <chrono>

int main() {
	v128b<float, 4> v1;
	v128b<float, 4> v2;

	glm::vec4 g1;
	glm::vec4 g2;

	auto v5 = v1 ^ v2;

	auto start = std::chrono::steady_clock::now();
	for (int i{}; i < 1000; i++) {
		v1[1] = 30.2f;
		v2[0] = 169.23f;
		v1[2] = 101.f;
		v2[2] = 50.5f;

		auto v3 = v1 + v2;

		v1 = v3;
	}
	std::cout << (std::chrono::steady_clock::now() - start) << '\n';

	start = std::chrono::steady_clock::now();
	for (int i{}; i < 1000; i++) {
		g1[1] = 30.2f;
		g2[0] = 169.23f;
		g1[2] = 101.f;
		g2[2] = 50.5f;

		auto g3 = g1 + g2;

		g1 = g3;
	}
	std::cout << (std::chrono::steady_clock::now() - start) << '\n';

	return 0;
}