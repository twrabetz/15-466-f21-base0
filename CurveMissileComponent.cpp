#include "CurveMissileComponent.hpp"
#include <math.h>

CurveMissileComponent::CurveMissileComponent( float danger )
{
	direction = ((float)rand() / RAND_MAX) > 0.5f ? 1 : -1;
	curveRate = ((float)rand() / RAND_MAX) * 0.125f + 0.125f + danger / 8;
}

void CurveMissileComponent::corePositionUpdate(float elapsed, float lifetime, glm::vec2& position, glm::vec2& velocity)
{
	float angle = atan2(velocity.y, velocity.x) + curveRate * elapsed * direction;
	velocity = glm::vec2(cos(angle), sin(angle)) * glm::length(velocity);
}