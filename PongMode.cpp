#include "PongMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

#include <iostream>

PongMode::PongMode() {

	srand((unsigned int)time(NULL));

	missiles.emplace_back(new Missile(1, court_radius));
	
	//----- allocate OpenGL resources -----
	{ //vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //vertex array mapping buffer for color_texture_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of PongMode::Vertex:
		glVertexAttribPointer(
			color_texture_program.Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 0 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program.Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Color_vec4);

		glVertexAttribPointer(
			color_texture_program.TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 + 4*1 //offset
		);
		glEnableVertexAttribArray(color_texture_program.TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //solid white texture:
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &white_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, white_tex);

		//upload a 1x1 image of solid white to the texture:
		glm::uvec2 size = glm::uvec2(1,1);
		std::vector< glm::u8vec4 > data(size.x*size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		//set filtering and wrapping parameters:
		//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}
}

PongMode::~PongMode() {

	//----- free OpenGL resources -----
	glDeleteBuffers(1, &vertex_buffer);
	vertex_buffer = 0;

	glDeleteVertexArrays(1, &vertex_buffer_for_color_texture_program);
	vertex_buffer_for_color_texture_program = 0;

	for (Missile* missile : missiles)
		delete missile;

	glDeleteTextures(1, &white_tex);
	white_tex = 0;
}

bool PongMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_MOUSEMOTION) {
		//convert mouse from window pixels (top-left origin, +y is down) to clip space ([-1,1]x[-1,1], +y is up):
		glm::vec2 clip_mouse = glm::vec2(
			(evt.motion.x + 0.5f) / window_size.x * 2.0f - 1.0f,
			(evt.motion.y + 0.5f) / window_size.y *-2.0f + 1.0f
		);
		playerTarget.x = clip_mouse.x * court_radius.x;
		playerTarget.y = clip_mouse.y * court_radius.y;
	}

	return false;
}

void PongMode::update(float elapsed) {

	lifetime += elapsed;

	if (lost_game)
		return;

	static std::mt19937 mt; //mersenne twister pseudo-random number generator

	//----- player update -----

	{ //Move player towards the mouse based on speed
		glm::vec2 diff = playerTarget - player;
		if( glm::length(diff) > 0 )
			player += glm::normalize(diff) * std::min(glm::length(diff), playerSpeed * elapsed);
	}

	//clamp player to court:

	player.x = std::max(player.x, -court_radius.x + player_radius.x);
	player.x = std::min(player.x, court_radius.x - player_radius.x);
	player.y = std::max(player.y, -court_radius.y + player_radius.y);
	player.y = std::min(player.y,  court_radius.y - player_radius.y);

	//----- create missiles -----
	if (lifetime > 5)
	{
		if (dangerAccumulationRate < maxDangerAccumulationRate)
			dangerAccumulationRate = std::min(maxDangerAccumulationRate, dangerAccumulationRate + dangerAccumulationIncreaseRate * elapsed);

		if (missileProbabilityPerSecond < maxMissileProbabilityPerSecond)
			missileProbabilityPerSecond = std::min(maxMissileProbabilityPerSecond, missileProbabilityPerSecond + missileProbabilityPerSecondIncreaseRate * elapsed);

		danger += elapsed * dangerAccumulationRate;

		float randomValue = ((float)rand()) / RAND_MAX;
		double frameProbability = 1 - pow(1 - missileProbabilityPerSecond, elapsed);
		if (randomValue < frameProbability || danger > 4)
		{
			if (deadMissiles.size() > 0)
			{
				int i = deadMissiles.front();
				delete missiles[i];
				missiles[i] = new Missile(danger, court_radius);
				deadMissiles.pop_front();
			}
			else
				missiles.emplace_back(new Missile(danger, court_radius));
			danger = 0;
		}
	}

	//----- missile updates -----
	
	for (int i = 0; i < missiles.size(); i++ )
	{
		if (missiles[i]->dead)
			continue;
		missiles[i]->update(elapsed);

		//---- collision handling ----

		//compute area of overlap:
		glm::vec2 min = glm::max(player - player_radius, missiles[i]->position - missiles[i]->radius);
		glm::vec2 max = glm::min(player + player_radius, missiles[i]->position + missiles[i]->radius);

		//collision with player
		if (min.x <= max.x && min.y <= max.y)
			lost_game = true;

		//court walls:
		if (missiles[i]->corePosition.y > court_radius.y - missiles[i]->radius.y || missiles[i]->corePosition.y < -court_radius.y + missiles[i]->radius.y ||
			missiles[i]->corePosition.x > court_radius.x - missiles[i]->radius.x || missiles[i]->corePosition.x < -court_radius.x + missiles[i]->radius.x) {
			missiles[i]->dead = true;
			deadMissiles.push_back(i);
		}

		//----- gradient trails -----

		//age up all locations in missile trail:
		for (auto& t : missiles[i]->trail) {
			t.z += elapsed;
		}
		//store fresh location at back of missile trail:
		missiles[i]->trail.emplace_back(missiles[i]->position, 0.0f);

		//trim any too-old locations from back of trail:
		//NOTE: since trail drawing interpolates between points, only removes back element if second-to-back element is too old:
		while (missiles[i]->trail.size() >= 2 && missiles[i]->trail[1].z > missiles[i]->trail_length) {
			missiles[i]->trail.pop_front();
		}
	}
}

void PongMode::draw(glm::uvec2 const &drawable_size) {
	//some nice colors from the course web page:
	#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
	const glm::u8vec4 bg_color = HEX_TO_U8VEC4(0x193b59ff);
	const glm::u8vec4 fg_color = HEX_TO_U8VEC4(0xf2d2b6ff);
	const glm::u8vec4 shadow_color = HEX_TO_U8VEC4(0xf2ad94ff);
	const glm::u8vec4 black = HEX_TO_U8VEC4(0x000000ff);
	const glm::u8vec4 fadedWhite = HEX_TO_U8VEC4(0xffffff22);
	#undef HEX_TO_U8VEC4

	//other useful drawing constants:
	const float wall_radius = 0.05f;
	const float shadow_offset = 0.07f;
	const float padding = 0.14f; //padding between outside of walls and edge of window

	//---- compute vertices to draw ----

	//vertices will be accumulated into this list and then uploaded+drawn at the end of this function:
	std::vector< Vertex > vertices;

	//inline helper function for rectangle drawing:
	auto draw_rectangle = [&vertices](glm::vec2 const &center, glm::vec2 const &radius, glm::u8vec4 const &color) {
		//draw rectangle as two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};

	glm::vec2 score_radius = glm::vec2(0.1f, 0.1f);

	if (lost_game)
	{
		draw_rectangle(glm::vec2(0, 0), glm::vec2(court_radius.x * 2, court_radius.y * 2), black);
	}
	else
	{

		//shadows for everything (except the trail):

		glm::vec2 s = glm::vec2(0.0f, -shadow_offset);

		draw_rectangle(glm::vec2(-court_radius.x - wall_radius, 0.0f) + s, glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), shadow_color);
		draw_rectangle(glm::vec2(court_radius.x + wall_radius, 0.0f) + s, glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), shadow_color);
		draw_rectangle(glm::vec2(0.0f, -court_radius.y - wall_radius) + s, glm::vec2(court_radius.x, wall_radius), shadow_color);
		draw_rectangle(glm::vec2(0.0f, court_radius.y + wall_radius) + s, glm::vec2(court_radius.x, wall_radius), shadow_color);
		draw_rectangle(player + s, player_radius, shadow_color);
		for (Missile* missile : missiles)
		{
			if (missile->dead)
				continue;

			//missile's trail:
			if (missile->trail.size() >= 2) {
				//start ti at second element so there is always something before it to interpolate from:
				std::deque< glm::vec3 >::iterator ti = missile->trail.begin() + 1;
				//draw trail from oldest-to-newest:
				constexpr uint32_t STEPS = 20;
				//draw from [STEPS, ..., 1]:
				for (uint32_t step = STEPS; step > 0; --step) {
					//time at which to draw the trail element:
					float t = step / float(STEPS) * missile->trail_length;

					//advance ti until 'just before' t:
					while (ti != missile->trail.end() && ti->z > t) ++ti;
					//if we ran out of recorded tail, stop drawing:
					if (ti == missile->trail.end()) break;
					//interpolate between previous and current trail point to the correct time:
					glm::vec3 a = *(ti - 1);
					glm::vec3 b = *(ti);
					glm::vec2 at = (t - a.z) / (b.z - a.z) * (glm::vec2(b) - glm::vec2(a)) + glm::vec2(a);

					//do the interpolation (casting to floating point vectors because glm::mix doesn't have an overload for u8 vectors):
					glm::u8vec4 color = glm::u8vec4(
						glm::mix(glm::vec4(missile->color), glm::vec4(fadedWhite), t / missile->trail_length)
					);

					//draw:
					draw_rectangle(at, missile->radius, color);
				}
			}
		}

		//solid objects:

		//walls:
		draw_rectangle(glm::vec2(-court_radius.x - wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
		draw_rectangle(glm::vec2(court_radius.x + wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
		draw_rectangle(glm::vec2(0.0f, -court_radius.y - wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);
		draw_rectangle(glm::vec2(0.0f, court_radius.y + wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);

		//paddles:
		draw_rectangle(player, player_radius, fg_color);


		//missiles:
		for( Missile* missile : missiles )
		{ 
			if (missile->dead)
				continue;
			draw_rectangle(missile->position, missile->radius, missile->main_color);
		}

		//scores:
		for (uint32_t i = 0; i < left_score; ++i) {
			draw_rectangle(glm::vec2(-court_radius.x + (2.0f + 3.0f * i) * score_radius.x, court_radius.y + 2.0f * wall_radius + 2.0f * score_radius.y), score_radius, fg_color);
		}
		for (uint32_t i = 0; i < right_score; ++i) {
			draw_rectangle(glm::vec2(court_radius.x - (2.0f + 3.0f * i) * score_radius.x, court_radius.y + 2.0f * wall_radius + 2.0f * score_radius.y), score_radius, fg_color);
		}
	}

	//------ compute court-to-window transform ------

	//compute area that should be visible:
	glm::vec2 scene_min = glm::vec2(
		-court_radius.x - 2.0f * wall_radius - padding,
		-court_radius.y - 2.0f * wall_radius - padding
	);
	glm::vec2 scene_max = glm::vec2(
		court_radius.x + 2.0f * wall_radius + padding,
		court_radius.y + 2.0f * wall_radius + 3.0f * score_radius.y + padding
	);

	//compute window aspect ratio:
	float aspect = drawable_size.x / float(drawable_size.y);
	//we'll scale the x coordinate by 1.0 / aspect to make sure things stay square.

	//compute scale factor for court given that...
	float scale = std::min(
		(2.0f * aspect) / (scene_max.x - scene_min.x), //... x must fit in [-aspect,aspect] ...
		(2.0f) / (scene_max.y - scene_min.y) //... y must fit in [-1,1].
	);

	glm::vec2 center = 0.5f * (scene_max + scene_min);

	//build matrix that scales and translates appropriately:
	glm::mat4 court_to_clip = glm::mat4(
		glm::vec4(scale / aspect, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, scale, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(-center.x * (scale / aspect), -center.y * scale, 0.0f, 1.0f)
	);
	//NOTE: glm matrices are specified in *Column-Major* order,
	// so each line above is specifying a *column* of the matrix(!)

	//also build the matrix that takes clip coordinates to court coordinates (used for mouse handling):
	clip_to_court = glm::mat3x2(
		glm::vec2(aspect / scale, 0.0f),
		glm::vec2(0.0f, 1.0f / scale),
		glm::vec2(center.x, center.y)
	);

	//---- actual drawing ----

	//clear the color buffer:
	glClearColor(bg_color.r / 255.0f, bg_color.g / 255.0f, bg_color.b / 255.0f, bg_color.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_texture_program as current program:
	glUseProgram(color_texture_program.program);

	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip));

	//use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_texture_program);

	//bind the solid white texture to location zero so things will be drawn just with their colors:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, white_tex);

	//run the OpenGL pipeline:
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

	//unbind the solid white texture:
	glBindTexture(GL_TEXTURE_2D, 0);

	//reset vertex array to none:
	glBindVertexArray(0);

	//reset current program to none:
	glUseProgram(0);
	

	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.

}
