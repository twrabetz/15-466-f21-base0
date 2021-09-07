#include "SineWeaveMissileComponent.hpp"
#include <math.h>
#include <iostream>

SineWeaveMissileComponent::SineWeaveMissileComponent(float danger)
{
	period = ((float)rand() / RAND_MAX) * 0.075f + 0.075f;
	magnitude = 0.5f + 1.0f * ((float)rand() / RAND_MAX);
}

void SineWeaveMissileComponent::update(float elapsed, float lifetime, glm::vec2& position, glm::vec2& velocity)
{
	position += glm::normalize(glm::vec2(velocity.y, -velocity.x)) * glm::length(velocity) / 4.0f * magnitude * sin(lifetime / period);
}