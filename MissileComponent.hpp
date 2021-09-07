#include <glm/glm.hpp>

#pragma once

struct MissileComponent
{	
	
	//This update needs to happen first, because the regular update will do things based on the updated core position
	virtual void corePositionUpdate(float elapsed, float lifetime, glm::vec2& position, glm::vec2& velocity) {}

	virtual void update(float elapsed, float lifetime, glm::vec2& position, glm::vec2& velocity) {}
	
};