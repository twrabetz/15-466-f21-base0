#include "MissileComponent.hpp"

struct SpeedChangeMissileComponent : MissileComponent
{

	float magnitude;
	float period;

	SpeedChangeMissileComponent(float danger);

	void update(float elapsed, float lifetime, glm::vec2& position, glm::vec2& velocity) override;

};