#include <limits>
#include <random>
#include <glm/glm.hpp>

struct urd_float_random {
	std::mt19937 gen;
	float m_min;
	float m_max;

	urd_float_random() : gen(std::random_device()() ), m_min{ 0.f }, m_max{ 1.f } {}
	urd_float_random(float s_min, float s_max) : gen(std::random_device()()), m_min{ s_min }, m_max{ s_max } {}

	float generate_float() {
		std::uniform_real_distribution<float> dis{ float(m_min), float(m_max) };
		return dis(gen);
	}
	glm::vec4 generate_vec4() {
		return glm::vec4{ generate_float(), generate_float(), generate_float(), generate_float() };
	}
	float generate_float(float min, float max) {
		std::uniform_real_distribution<float> dis(min, max);
		return dis(gen);
	}
	glm::vec4 generate_vec4(float min, float max) {
		return glm::vec4{ generate_float(min, max), generate_float(min, max), generate_float(min, max), generate_float(min, max) };
	}
};
