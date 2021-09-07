#include "MissileComponent.hpp"

struct SineWeaveMissileComponent : MissileComponent
{

	float period;
	float magnitude;

	SineWeaveMissileComponent(float danger);

	void update(float elapsed, float lifetime, glm::vec2& position, glm::vec2& velocity) override;

};