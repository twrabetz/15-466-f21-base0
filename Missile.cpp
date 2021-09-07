#include "Missile.hpp"
#include "SineWeaveMissileComponent.hpp"
#include "CurveMissileComponent.hpp"
#include "SpeedChangeMissileComponent.hpp"
#include <iostream>

Missile::Missile(float danger, glm::vec2 court_radius)
{
	radius = glm::vec2(0.075f + 0.075f * danger, 0.075f + 0.075f * danger);
	glm::vec2 randomBox = glm::vec2(((float)rand() / RAND_MAX) * 2 - 1, ((float)rand() / RAND_MAX) * 2 - 1);
	float xRatio;
	float yRatio;
	if ((float)rand() / RAND_MAX < 0.5f)
	{
		xRatio = randomBox.x > 0 ? 1.0f : -1.0f;
		yRatio = randomBox.y;
	}
	else
	{
		xRatio = randomBox.x;
		yRatio = randomBox.y > 0 ? 1.0f : -1.0f;
	}
	position = glm::vec2((court_radius.x - radius.x - 0.01f) * xRatio, (court_radius.y - radius.y - 0.01f) * yRatio);
	corePosition = position;
	float speed = 2 + danger;
	velocity = glm::normalize(-position + (randomBox * 2.5f)) * speed;
	coreVelocity = velocity;

	//Set up trail as if missile has been here forever
	trail.clear();
	trail.emplace_back(position, trail_length);
	trail.emplace_back(position, 0.0f);

	int numComponents = std::min(3, (int)(((float)rand() / RAND_MAX) * 4));

#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))

	int hex = 0;

	switch (numComponents)
	{
	case 0:
	default:
		hex = 0x88888888;
		break;
	case 1:
	{
		float num = ((float)rand() / RAND_MAX);
		if (num < 0.33f)
		{
			components.push_back(new CurveMissileComponent(danger));
			hex = 0xdd000088;
		}
		else if (num < 0.66f)
		{
			components.push_back(new SineWeaveMissileComponent(danger));
			hex = 0x00dd0088;
		}
		else
		{
			components.push_back(new SpeedChangeMissileComponent(danger));
			hex = 0x0000dd88;
		}
		break;
	}
	case 2:
	{
		float num = ((float)rand() / RAND_MAX);
		if (num < 0.33f)
		{
			components.push_back(new CurveMissileComponent(danger));
			components.push_back(new SineWeaveMissileComponent(danger));
			hex = 0xdddd0088;
		}
		else if (num < 0.66f)
		{
			components.push_back(new SineWeaveMissileComponent(danger));
			components.push_back(new SpeedChangeMissileComponent(danger));
			hex = 0x00dddd88;
		}
		else
		{
			components.push_back(new SpeedChangeMissileComponent(danger));
			components.push_back(new CurveMissileComponent(danger));
			hex = 0xdd00dd88;
		}
		break;
	}
	case 3:
		components.push_back(new CurveMissileComponent(danger));
		components.push_back(new SineWeaveMissileComponent(danger));
		components.push_back(new SpeedChangeMissileComponent(danger));
		hex = 0xdddddd88;
		break;
	}

	color = HEX_TO_U8VEC4(hex);
	main_color = HEX_TO_U8VEC4((hex | 0x000000ff));

#undef HEX_TO_U8VEC4
}

Missile::~Missile()
{
	for (MissileComponent* component : components)
		delete component;
}

void Missile::update(float elapsed)
{
	lifetime += elapsed;
	corePosition += elapsed * velocity;
	position = corePosition;
	velocity = coreVelocity;

	for (MissileComponent* component : components)
	{
		component->corePositionUpdate(elapsed, lifetime, corePosition, coreVelocity);
	}
	for (MissileComponent* component : components)
	{
		component->update(elapsed, lifetime, position, velocity);
	}
}