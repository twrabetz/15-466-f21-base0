#include "MissileComponent.hpp"

struct CurveMissileComponent : MissileComponent
{

	int direction;
	float curveRate;

	CurveMissileComponent(float danger);

	void corePositionUpdate(float elapsed, float lifetime, glm::vec2& position, glm::vec2& velocity) override;

};