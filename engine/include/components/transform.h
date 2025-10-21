#ifndef ENGINE_TRANSFORM_H
#define ENGINE_TRANSFORM_H

#include <glm/glm.hpp>


struct PositionComponent {
	glm::vec3 m_WorldPos;
};

struct ScaleComponent {
	glm::vec3 m_Scale;
};

struct RotationComponent {
	glm::vec3 m_Rotation; // Euler angles in degrees
};

/*
 *RegisterReflectionComponent<Simple>(
            "Simple",
            MemberRegistrationV<&Simple::a, "a">,
            MemberRegistrationV<&Simple::b, "b">
        );
 */


#endif