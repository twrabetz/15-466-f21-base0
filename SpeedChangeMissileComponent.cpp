#include "SpeedChangeMissileComponent.hpp"
#include <math.h>
#include <iostream>

SpeedChangeMissileComponent::SpeedChangeMissileComponent(float danger)
{
	magnitude = ((float)rand() / RAND_MAX) * 1.0f + 1.0f + danger * 1.5f;
	period = 0.2f;
}

void SpeedChangeMissileComponent::update(float elapsed, float lifetime, glm::vec2& position, glm::vec2& velocity)
{
	velocity = glm::normalize(velocity) * (glm::length(velocity) + magnitude * sin(lifetime / period));
}