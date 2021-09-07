#include <glm/glm.hpp>

#include <deque>
#include <vector>
#include "MissileComponent.hpp"

struct Missile
{
	glm::vec2 radius;
	glm::vec2 position;
	glm::vec2 corePosition;
	glm::vec2 velocity;
	glm::vec2 coreVelocity;

	bool dead = false;
	
	//----- pretty gradient trails -----

	float trail_length = 1.3f;
	std::deque< glm::vec3 > trail; //stores (x,y,age), oldest elements first

	std::vector<MissileComponent*> components;
	float lifetime = 0;

	glm::u8vec4 color;
	glm::u8vec4 main_color;

	Missile( float danger, glm::vec2 court_radius );
	~Missile();

	void update(float elapsed);
};