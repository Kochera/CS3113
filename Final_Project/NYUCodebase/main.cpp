#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <math.h>
#include "ShaderProgram.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <SDL_mixer.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <vector>
#define FIXED_TIMESTEP 0.0166666f 
#define MAX_TIMESTEPS 6

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;
enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_WIN, STATE_GAME_LEVEL2, STATE_GAME_LEVEL3, STATE_GAME_OVER };
GameMode mode = STATE_MAIN_MENU;
float lerp(float v0, float v1, float t) {
	return (1.0-t)*v0 + v1 * t;
}
GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);

	if (image == NULL) {
		std::cout << "Unable to load image";
		assert(false);
	}
	GLuint retTexture;
	glGenTextures(1, &retTexture);

	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(image);
	return retTexture;
}


Mix_Chunk *someSound;
GLuint characters;
GLuint World;
GLuint Letters;
float friction_x=0.5f;
float friction_y=0.5f;
class SheetSprite {
public:
	SheetSprite() {}
	SheetSprite(unsigned int textureID, int index, int spriteCountX, int spriteCountY) : textureID(textureID), index(index), spriteCountX(spriteCountX), spriteCountY(spriteCountY) {}
	void Draw(ShaderProgram &program);
	float size;
	unsigned int textureID;
	int index;
	int spriteCountX;
	int spriteCountY;
	float width = 1.0 / (float)spriteCountX;
	float height = 1.0 / (float)spriteCountY;
};

void SheetSprite::Draw(ShaderProgram &program) {
	glBindTexture(GL_TEXTURE_2D, textureID);
	float u = (float)(((int)index) % spriteCountX) / (float)spriteCountX;
	float v = (float)(((int)index) / spriteCountX) / (float)spriteCountY;
	float spriteWidth = 1.0 / (float)spriteCountX;
	float spriteHeight = 1.0 / (float)spriteCountY;
	float texCoords[] = { u, v + spriteHeight,
		u + spriteWidth, v,
		u, v,
		u + spriteWidth, v,
		u, v + spriteHeight,
		u + spriteWidth, v + spriteHeight };
	
	float vertices[] = { -0.5f*size, -0.5f*size, 0.5f*size, 0.5f*size, -0.5f*size, 0.5f*size, 0.5f*size, 0.5f*size,  -0.5f*size, -0.5f*size, 0.5f*size, -0.5f*size };

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);
	
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);

}


enum EntityType { ENTITY_PLAYER, ENTITY_ENEMY, ENTITY_COIN };
class Entity {
public:
	void Draw(ShaderProgram &program) {

		//Players Ship
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		glBindTexture(GL_TEXTURE_2D, characters);
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(position.x, position.y, 0.0f));
		program.SetModelMatrix(modelMatrix);

		sprite.Draw(program);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

	};
	SheetSprite sprite;

	glm::vec3 position;
	glm::vec3 size;
	glm::vec3 velocity;
	glm::vec3 acceleration;

	

	bool isStatic;
	EntityType entityType;

	bool collidedTop;
	bool collidedBottom;
	bool collidedLeft;
	bool collidedRight;
};

void DrawText(ShaderProgram &program, GLuint fontTexture, std::string text, float size, float spacing, float move_x, float move_y) {
	float character_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;
	for (int i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];
		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;
		vertexData.insert(vertexData.end(), { ((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size, });
		texCoordData.insert(texCoordData.end(), { texture_x, texture_y,
			texture_x, texture_y + character_size,
			texture_x + character_size, texture_y,
			texture_x + character_size, texture_y + character_size,
			texture_x + character_size, texture_y,
			texture_x, texture_y + character_size, });
	}
	glm::mat4 modelMatrix = glm::mat4(1.0f);

	glBindTexture(GL_TEXTURE_2D, fontTexture);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(move_x, move_y, 0.0f));
	program.SetModelMatrix(modelMatrix);

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program.positionAttribute);

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program.texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}




class GameState {
public:
	Entity p1;
	Entity e1;
	std::vector<Entity> floors;
	std::vector<Entity> blocks;
	Entity rmid;
	Entity lmid;
	Entity top;
	Entity offmid;
	Entity hill;
};
GameState state1;
GameState state2;
GameState state3;
void RenderMainMenu(ShaderProgram &program) {
	DrawText(program, Letters, "Platformer", 0.2f, 0.0f, -1.0f, 0.0f);
	DrawText(program, Letters, "(Press y to start)", 0.1f, 0.0f, -0.85f, -0.3f);
	DrawText(program, Letters, "(Reach the coin to win, press Q to quit.)", 0.08f, 0.0f, -1.6f, -0.5f);
}
void RenderGameOver(ShaderProgram &program) {
	DrawText(program, Letters, "Game Over", 0.2f, 0.0f, -0.8f, 0.0f);
	DrawText(program, Letters, "You Suck, but it's okay", 0.08f, 0.0f, -0.9f, -0.3f);
}
void RenderGameWin(ShaderProgram &program) {
	DrawText(program, Letters, "YOU WIN!", 0.2f, 0.0f, -0.7f, 0.0f);
	
}
SheetSprite player;
glm::mat4 viewMatrix;
void RenderGameLevel(ShaderProgram &program) {
	//player
	
	state1.p1.Draw(program);

	//enemy
	state1.e1.Draw(program);

	//top
	state1.top.Draw(program);

	//floor
	for (int i = 0; i < state1.floors.size(); i++) {
		state1.floors[i].Draw(program);
	}
	
	state1.rmid.Draw(program);
	state1.lmid.Draw(program);

	
	state1.hill.Draw(program);

}

void RenderGameLevel2(ShaderProgram &program) {
	//player
	state2.p1.Draw(program);

	//enemy
	state2.e1.Draw(program);

	//top
	state2.top.Draw(program);

	//floor
	for (int i = 0; i < state2.floors.size(); i++) {
		state2.floors[i].Draw(program);
	}
	//Blocks
	for (int i = 0; i < state2.blocks.size(); i++) {
		state2.blocks[i].Draw(program);
	}
	state2.rmid.Draw(program);
	state2.lmid.Draw(program);


	state2.hill.Draw(program);

}

void RenderGameLevel3(ShaderProgram &program) {
	//player
	state3.p1.Draw(program);

	//enemy
	state3.e1.Draw(program);

	//top
	state3.top.Draw(program);

	//floor
	for (int i = 0; i < state3.floors.size(); i++) {
		state3.floors[i].Draw(program);
	}

	//Blocks
	for (int i = 0; i < state3.blocks.size(); i++) {
		state3.blocks[i].Draw(program);
	}

	state3.rmid.Draw(program);
	state3.lmid.Draw(program);


	state3.hill.Draw(program);

}



void UpdateMainMenu(float elapsed) {

}
void UpdateGameWin(float elapsed) {

}


void ProcessGameLevelInput(float elapsed, GameState &state) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_RIGHT]) {
		state.p1.acceleration.x = 1.0f;
		
	}
	else if (keys[SDL_SCANCODE_LEFT]) {
		state.p1.acceleration.x = -1.0f;
		
	}

	if (keys[SDL_SCANCODE_Q]) {
		mode = STATE_GAME_OVER;
	}

	if (keys[SDL_SCANCODE_SPACE] && state.p1.collidedBottom == true) {
		state.p1.collidedBottom = false;
		state.p1.velocity.y = 1.9f;
	}
}
void UpdateGameLevel(float elapsed, GameState &state) {
	state.p1.acceleration.x = 0.0f;
	ProcessGameLevelInput(elapsed, state);
	float penetration = 0.0f;
	float penetration_e = 0.0f;
	state.p1.velocity.x = lerp(state.p1.velocity.x, 0.0f, elapsed*friction_x);
	state.p1.velocity.x += state.p1.acceleration.x*elapsed;
	state.p1.position.x += state.p1.velocity.x*elapsed;


	state.e1.position.x += state.e1.velocity.x*elapsed;
	
	if (state.e1.position.x >= state.p1.position.x && state.e1.velocity.x >0) {
		state.e1.velocity.x *= -1.0f;
	}
	else if (state.e1.position.x < state.p1.position.x && state.e1.velocity.x <0 ) {
		state.e1.velocity.x *= -1.0f;
	}
	float p_x2 = fabs(state.p1.position.x - state.top.position.x) - ((state.p1.size.x + state.top.size.x) / 2.0f);
	float p_y2 = fabs(state.p1.position.y - state.top.position.y) - ((state.p1.size.y + state.top.size.y) / 2.0f);

	if (p_x2 < 0 && p_y2 < 0) {
		if (state.p1.position.x > state.top.position.x+state.top.size.x/2.0f) {
			penetration = fabs((state.p1.position.x - state.top.position.x) - state.p1.size.x / 2.0f - state.top.size.x / 2.0f);
			state.p1.position.x += penetration;
			state.p1.collidedBottom= true;
		}
		else if (state.p1.position.x < state.top.position.x - state.top.size.x/2.0f) {
			penetration = fabs((state.top.position.x - state.p1.position.x) - state.p1.size.x / 2.0f - state.top.size.x / 2.0f);
			state.p1.position.x -= penetration;
			state.p1.collidedTop = true;
		}
	}
	
	float p_x3 = fabs(state.p1.position.x - state.rmid.position.x) - ((state.p1.size.x + state.rmid.size.x) / 2.0f);
	float p_y3 = fabs(state.p1.position.y - state.rmid.position.y) - ((state.p1.size.y + state.rmid.size.y) / 2.0f);

	if (p_x3 < 0 && p_y3 < 0) {
		if (state.p1.position.x > state.rmid.position.x + state.rmid.size.x/2.0f) {
			penetration = fabs((state.p1.position.x - state.rmid.position.x) - state.p1.size.x / 2.0f - state.rmid.size.x / 2.0f);
			state.p1.position.x += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.x < state.rmid.position.x - state.rmid.size.x / 2.0f) {
			penetration = fabs((state.rmid.position.x - state.p1.position.x) - state.p1.size.x / 2.0f - state.rmid.size.x / 2.0f);
			state.p1.position.x -= penetration;
			state.p1.collidedTop = true;
		}
	}

	float p_x8 = fabs(state.p1.position.x - state.lmid.position.x) - ((state.p1.size.x + state.lmid.size.x) / 2.0f);
	float p_y8 = fabs(state.p1.position.y - state.lmid.position.y) - ((state.p1.size.y + state.lmid.size.y) / 2.0f);

	if (p_x8 < 0 && p_y8 < 0) {
		if (state.p1.position.x > state.lmid.position.x +state.lmid.size.x/2.0f) {
			penetration = fabs((state.p1.position.x - state.lmid.position.x) - state.p1.size.x / 2.0f - state.lmid.size.x / 2.0f);
			state.p1.position.x += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.x < state.lmid.position.x - state.lmid.size.x / 2.0f) {
			penetration = fabs((state.lmid.position.x - state.p1.position.x) - state.p1.size.x / 2.0f - state.lmid.size.x / 2.0f);
			state.p1.position.x -= penetration;
			state.p1.collidedTop = true;
		}
	}

	float p_x9 = fabs(state.p1.position.x - state.hill.position.x) - ((state.p1.size.x + state.hill.size.x) / 2.0f);
	float p_y9 = fabs(state.p1.position.y - state.hill.position.y) - ((state.p1.size.y + state.hill.size.y) / 2.0f);
	float e_x9 = fabs(state.e1.position.x - state.hill.position.x) - ((state.e1.size.x + state.hill.size.x) / 2.0f);
	float e_y9 = fabs(state.e1.position.y - state.hill.position.y) - ((state.e1.size.y + state.hill.size.y) / 2.0f);

	if (p_x9 < 0 && p_y9 < 0) {
		if (state.p1.position.x > state.hill.position.x + state.hill.size.x/2.0f) {
			penetration = fabs((state.p1.position.x - state.hill.position.x) - state.p1.size.x / 2.0f - state.hill.size.x / 2.0f);
			state.p1.position.x += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.x < state.hill.position.x - state.hill.size.x / 2.0f) {
			penetration = fabs((state.hill.position.x - state.p1.position.x) - state.p1.size.x / 2.0f - state.hill.size.x / 2.0f);
			state.p1.position.x -= penetration;
			state.p1.collidedTop = true;
		}
	}
	if (e_x9 < 0 && e_y9 < 0) {
		//collision with bottom of sprite
		if (state.e1.position.x > state.hill.position.x + state.hill.size.x / 2.0f) {
			penetration = fabs((state.e1.position.x - state.hill.position.x) - state.e1.size.x / 2.0f - state.hill.size.x / 2.0f);
			state.e1.position.x += penetration;
			state.e1.collidedBottom = true;
		}
		//collision with top of sprite
		else if (state.e1.position.x < state.hill.position.x - state.hill.size.x / 2.0f) {
			penetration = fabs((state.hill.position.x - state.e1.position.x) - state.e1.size.x / 2.0f - state.hill.size.x / 2.0f);
			state.e1.position.x -= penetration;
			state.e1.collidedTop = true;
		}
	}

	state.p1.acceleration.y = 0.0f;
	state.p1.acceleration.y = -1.5f;
	state.p1.velocity.y = lerp(state.p1.velocity.y, 0.0f, elapsed*friction_y);
	state.p1.velocity.y += state.p1.acceleration.y*elapsed;
	state.p1.position.y += state.p1.velocity.y*elapsed;
	
	
	state.p1.collidedBottom = false;
	state.p1.collidedTop = false;
	state.p1.collidedRight = false;
	state.p1.collidedLeft = false;
	//enemy Movement
	state.e1.acceleration.y = 0.0f;
	state.e1.acceleration.y = -1.5f;
	state.e1.velocity.y = lerp(state.e1.velocity.y, 0.0f, elapsed*friction_y);
	state.e1.velocity.y += state.e1.acceleration.y*elapsed;
	state.e1.position.y += state.e1.velocity.y*elapsed;
	
	
	
	state.e1.collidedBottom = false;
	state.e1.collidedTop = false;
	state.e1.collidedRight = false;
	state.e1.collidedLeft = false;

	
	//floor
	for (int i = 0; i < state.floors.size(); i++) {
		float p_x = fabs(state.p1.position.x - state.floors[i].position.x) - ((state.p1.size.x + state.floors[i].size.x) / 2.0f);
		float p_y = fabs(state.p1.position.y - state.floors[i].position.y) - ((state.p1.size.y + state.floors[i].size.y) / 2.0f);
		float e_x = fabs(state.e1.position.x - state.floors[i].position.x) - ((state.e1.size.x + state.floors[i].size.x) / 2.0f);
		float e_y = fabs(state.e1.position.y - state.floors[i].position.y) - ((state.e1.size.y + state.floors[i].size.y) / 2.0f);

		if (p_x <= 0 && p_y <= 0 ) {
			penetration = fabs((state.p1.position.y - state.floors[i].position.y) - state.p1.size.y / 2.0f - state.floors[i].size.y/2.0f);
			state.p1.position.y += penetration;
			state.p1.collidedBottom = true;
		}
		if (e_x <= 0 && e_y <= 0) {
			penetration_e = fabs((state.e1.position.y - state.floors[i].position.y) - state.e1.size.y / 2.0f - state.floors[i].size.y / 2.0f);
			state.e1.position.y += penetration_e;
			state.e1.collidedBottom = true;
		}
	}

	//top
	float p_x4 = fabs(state.p1.position.x - state.top.position.x) - ((state.p1.size.x + state.top.size.x) / 2.0f);
	float p_y4 = fabs(state.p1.position.y - state.top.position.y) - ((state.p1.size.y + state.top.size.y) / 2.0f);

	if (p_x4 < 0 && p_y4 < 0) {
		if (state.p1.position.y > state.top.position.y+state.top.size.y/2.0f) {
			penetration = fabs((state.p1.position.y - state.top.position.y) - state.p1.size.y / 2.0f - state.top.size.y / 2.0f);
			state.p1.position.y += penetration;
			state.p1.collidedBottom = true;
			Mix_PlayChannel(-1, someSound, 0);
			mode = STATE_GAME_LEVEL2;
		}
		else if (state.p1.position.y < state.top.position.y - state.top.size.y / 2.0f) {
			penetration = fabs((state.top.position.y - state.p1.position.y) - state.p1.size.y / 2.0f - state.top.size.y / 2.0f);
			state.p1.position.y -= penetration;
			state.p1.collidedTop = true;
		}
	}
	
	
	//rmid
	float p_x5 = fabs(state.p1.position.x - state.rmid.position.x) - ((state.p1.size.x + state.rmid.size.x) / 2.0f);
	float p_y5 = fabs(state.p1.position.y - state.rmid.position.y) - ((state.p1.size.y + state.rmid.size.y) / 2.0f);
	if (p_x5 < 0 && p_y5 < 0) {
		if (state.p1.position.y > state.rmid.position.y+state.rmid.size.y/2.0f) {
			penetration = fabs((state.p1.position.y - state.rmid.position.y) - state.p1.size.y / 2.0f - state.rmid.size.y / 2.0f);
			state.p1.position.y += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.y < state.rmid.position.y - state.rmid.size.y / 2.0f) {
			penetration = fabs((state.rmid.position.y - state.p1.position.y) - state.p1.size.y / 2.0f - state.rmid.size.y / 2.0f);
			state.p1.position.y -= penetration;
			state.p1.collidedTop = true;
		}
	}
	

	//lmid
	float p_x6 = fabs(state.p1.position.x - state.lmid.position.x) - ((state.p1.size.x + state.lmid.size.x) / 2.0f);
	float p_y6 = fabs(state.p1.position.y - state.lmid.position.y) - ((state.p1.size.y + state.lmid.size.y) / 2.0f);

	if (p_x6 < 0 && p_y6 < 0) {
		if (state.p1.position.y > state.lmid.position.y+state.lmid.size.y/2.0f) {
			penetration = fabs((state.p1.position.y - state.lmid.position.y) - state.p1.size.y / 2.0f - state.lmid.size.y / 2.0f);
			state.p1.position.y += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.y < state.lmid.position.y - state.lmid.size.y / 2.0f) {
			penetration = fabs((state.lmid.position.y - state.p1.position.y) - state.p1.size.y / 2.0f - state.lmid.size.y / 2.0f);
			state.p1.position.y -= penetration;
			state.p1.collidedTop = true;
		}
	}

	//hill
	float p_x7 = fabs(state.p1.position.x - state.hill.position.x) - ((state.p1.size.x + state.hill.size.x) / 2.0f);
	float p_y7 = fabs(state.p1.position.y - state.hill.position.y) - ((state.p1.size.y + state.hill.size.y) / 2.0f);

	if (p_x7 < 0 && p_y7 < 0) {
		if (state.p1.position.y > state.hill.position.y+state.hill.size.y/2.0f) {
			penetration = fabs((state.p1.position.y - state.hill.position.y) - state.p1.size.y / 2.0f - state.hill.size.y / 2.0f);
			state.p1.position.y += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.y < state.hill.position.y - state.hill.size.y/2.0f) {
			penetration = fabs((state.hill.position.y - state.p1.position.y) - state.p1.size.y / 2.0f - state.hill.size.y / 2.0f);
			state.p1.position.y -= penetration;
			state.p1.collidedTop = true;
		}
	}
	

	

	//Snake knocks player around
	float p_ex = fabs(state.p1.position.x - state.e1.position.x) - ((state.p1.size.x + state.e1.size.x) / 2);
	float p_ey = fabs(state.p1.position.y - state.e1.position.y) - ((state.p1.size.y + state.e1.size.y) / 2);
	if (p_ex < 0 && p_ey < 0) {
		mode = STATE_GAME_OVER;
	}
}

void UpdateGameLevel2(float elapsed, GameState &state) {
	state.p1.acceleration.x = 0.0f;
	ProcessGameLevelInput(elapsed, state);
	float penetration = 0.0f;
	float penetration_e = 0.0f;
	state.p1.velocity.x = lerp(state.p1.velocity.x, 0.0f, elapsed*friction_x);
	state.p1.velocity.x += state.p1.acceleration.x*elapsed;
	state.p1.position.x += state.p1.velocity.x*elapsed;


	state.e1.position.x += state.e1.velocity.x*elapsed;

	if (state.e1.position.x >= state.p1.position.x && state.e1.velocity.x > 0) {
		state.e1.velocity.x *= -1.0f;
	}
	else if (state.e1.position.x < state.p1.position.x && state.e1.velocity.x < 0) {
		state.e1.velocity.x *= -1.0f;
	}
	float p_x2 = fabs(state.p1.position.x - state.top.position.x) - ((state.p1.size.x + state.top.size.x) / 2.0f);
	float p_y2 = fabs(state.p1.position.y - state.top.position.y) - ((state.p1.size.y + state.top.size.y) / 2.0f);

	if (p_x2 < 0 && p_y2 < 0) {
		if (state.p1.position.x > state.top.position.x + state.top.size.x / 2.0f) {
			penetration = fabs((state.p1.position.x - state.top.position.x) - state.p1.size.x / 2.0f - state.top.size.x / 2.0f);
			state.p1.position.x += penetration;
			state.p1.collidedLeft = true;
		}
		else if (state.p1.position.x < state.top.position.x - state.top.size.x / 2.0f) {
			penetration = fabs((state.top.position.x - state.p1.position.x) - state.p1.size.x / 2.0f - state.top.size.x / 2.0f);
			state.p1.position.x -= penetration;
			state.p1.collidedRight = true;
		}
	}

	//blocks x's
	for (int j = 0; j < state.blocks.size(); j++) {
		float p_x13 = fabs(state.p1.position.x - state.blocks[j].position.x) - ((state.p1.size.x + state.blocks[j].size.x) / 2.0f);
		float p_y13 = fabs(state.p1.position.y - state.blocks[j].position.y) - ((state.p1.size.y + state.blocks[j].size.y) / 2.0f);


		if (p_x13 <= 0 && p_y13 <= 0) {
			if (state.p1.position.x > state.blocks[j].position.x + state.blocks[j].size.x / 2.0f) {
				penetration = fabs((state.p1.position.x - state.blocks[j].position.x) - state.p1.size.x / 2.0f - state.blocks[j].size.x / 2.0f);
				state.p1.position.x += penetration;
				state.p1.collidedBottom = true;
			}
			else if (state.p1.position.x < state.blocks[j].position.x - state.blocks[j].size.x / 2.0f) {
				penetration = fabs((state.blocks[j].position.x - state.p1.position.x) - state.p1.size.x / 2.0f - state.blocks[j].size.x / 2.0f);
				state.p1.position.x -= penetration;
				state.p1.collidedTop = true;
			}
		}
		

	}

	float p_x3 = fabs(state.p1.position.x - state.rmid.position.x) - ((state.p1.size.x + state.rmid.size.x) / 2.0f);
	float p_y3 = fabs(state.p1.position.y - state.rmid.position.y) - ((state.p1.size.y + state.rmid.size.y) / 2.0f);

	if (p_x3 < 0 && p_y3 < 0) {
		if (state.p1.position.x > state.rmid.position.x + state.rmid.size.x / 2.0f) {
			penetration = fabs((state.p1.position.x - state.rmid.position.x) - state.p1.size.x / 2.0f - state.rmid.size.x / 2.0f);
			state.p1.position.x += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.x < state.rmid.position.x - state.rmid.size.x / 2.0f) {
			penetration = fabs((state.rmid.position.x - state.p1.position.x) - state.p1.size.x / 2.0f - state.rmid.size.x / 2.0f);
			state.p1.position.x -= penetration;
			state.p1.collidedTop = true;
		}
	}

	float p_x8 = fabs(state.p1.position.x - state.lmid.position.x) - ((state.p1.size.x + state.lmid.size.x) / 2.0f);
	float p_y8 = fabs(state.p1.position.y - state.lmid.position.y) - ((state.p1.size.y + state.lmid.size.y) / 2.0f);

	if (p_x8 < 0 && p_y8 < 0) {
		if (state.p1.position.x > state.lmid.position.x + state.lmid.size.x / 2.0f) {
			penetration = fabs((state.p1.position.x - state.lmid.position.x) - state.p1.size.x / 2.0f - state.lmid.size.x / 2.0f);
			state.p1.position.x += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.x < state.lmid.position.x - state.lmid.size.x / 2.0f) {
			penetration = fabs((state.lmid.position.x - state.p1.position.x) - state.p1.size.x / 2.0f - state.lmid.size.x / 2.0f);
			state.p1.position.x -= penetration;
			state.p1.collidedTop = true;
		}
	}

	float p_x9 = fabs(state.p1.position.x - state.hill.position.x) - ((state.p1.size.x + state.hill.size.x) / 2.0f);
	float p_y9 = fabs(state.p1.position.y - state.hill.position.y) - ((state.p1.size.y + state.hill.size.y) / 2.0f);
	float e_x9 = fabs(state.e1.position.x - state.hill.position.x) - ((state.e1.size.x + state.hill.size.x) / 2.0f);
	float e_y9 = fabs(state.e1.position.y - state.hill.position.y) - ((state.e1.size.y + state.hill.size.y) / 2.0f);

	if (p_x9 < 0 && p_y9 < 0) {
		if (state.p1.position.x > state.hill.position.x + state.hill.size.x / 2.0f) {
			penetration = fabs((state.p1.position.x - state.hill.position.x) - state.p1.size.x / 2.0f - state.hill.size.x / 2.0f);
			state.p1.position.x += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.x < state.hill.position.x - state.hill.size.x / 2.0f) {
			penetration = fabs((state.hill.position.x - state.p1.position.x) - state.p1.size.x / 2.0f - state.hill.size.x / 2.0f);
			state.p1.position.x -= penetration;
			state.p1.collidedTop = true;
		}
	}
	if (e_x9 < 0 && e_y9 < 0) {
		//collision with bottom of sprite
		if (state.e1.position.x > state.hill.position.x + state.hill.size.x / 2.0f) {
			penetration = fabs((state.e1.position.x - state.hill.position.x) - state.e1.size.x / 2.0f - state.hill.size.x / 2.0f);
			state.e1.position.x += penetration;
			state.e1.collidedBottom = true;
		}
		//collision with top of sprite
		else if (state.e1.position.x < state.hill.position.x - state.hill.size.x / 2.0f) {
			penetration = fabs((state.hill.position.x - state.e1.position.x) - state.e1.size.x / 2.0f - state.hill.size.x / 2.0f);
			state.e1.position.x -= penetration;
			state.e1.collidedTop = true;
		}
	}

	state.p1.acceleration.y = 0.0f;
	state.p1.acceleration.y = -1.5f;
	state.p1.velocity.y = lerp(state.p1.velocity.y, 0.0f, elapsed*friction_y);
	state.p1.velocity.y += state.p1.acceleration.y*elapsed;
	state.p1.position.y += state.p1.velocity.y*elapsed;


	state.p1.collidedBottom = false;
	state.p1.collidedTop = false;
	state.p1.collidedRight = false;
	state.p1.collidedLeft = false;
	//enemy Movement
	state.e1.acceleration.y = 0.0f;
	state.e1.acceleration.y = -1.5f;
	state.e1.velocity.y = lerp(state.e1.velocity.y, 0.0f, elapsed*friction_y);
	state.e1.velocity.y += state.e1.acceleration.y*elapsed;
	state.e1.position.y += state.e1.velocity.y*elapsed;



	state.e1.collidedBottom = false;
	state.e1.collidedTop = false;
	state.e1.collidedRight = false;
	state.e1.collidedLeft = false;


	//floor
	for (int i = 0; i < state.floors.size(); i++) {
		float p_x = fabs(state.p1.position.x - state.floors[i].position.x) - ((state.p1.size.x + state.floors[i].size.x) / 2.0f);
		float p_y = fabs(state.p1.position.y - state.floors[i].position.y) - ((state.p1.size.y + state.floors[i].size.y) / 2.0f);
		float e_x = fabs(state.e1.position.x - state.floors[i].position.x) - ((state.e1.size.x + state.floors[i].size.x) / 2.0f);
		float e_y = fabs(state.e1.position.y - state.floors[i].position.y) - ((state.e1.size.y + state.floors[i].size.y) / 2.0f);

		if (p_x <= 0 && p_y <= 0) {
			penetration = fabs((state.p1.position.y - state.floors[i].position.y) - state.p1.size.y / 2.0f - state.floors[i].size.y / 2.0f);
			state.p1.position.y += penetration;
			state.p1.collidedBottom = true;
		}
		if (e_x <= 0 && e_y <= 0) {
			penetration_e = fabs((state.e1.position.y - state.floors[i].position.y) - state.e1.size.y / 2.0f - state.floors[i].size.y / 2.0f);
			state.e1.position.y += penetration_e;
			state.e1.collidedBottom = true;
		}
	}

	//blocks
	for (int j = 0; j < state.blocks.size(); j++) {
		float p_x12 = fabs(state.p1.position.x - state.blocks[j].position.x) - ((state.p1.size.x + state.blocks[j].size.x) / 2.0f);
		float p_y12 = fabs(state.p1.position.y - state.blocks[j].position.y) - ((state.p1.size.y + state.blocks[j].size.y) / 2.0f);


		if (p_x12 <= 0 && p_y12 <= 0) {
			if (state.p1.position.y > state.blocks[j].position.y + state.blocks[j].size.y / 2.0f) {
				penetration = fabs((state.p1.position.y - state.blocks[j].position.y) - state.p1.size.y / 2.0f - state.blocks[j].size.y / 2.0f);
				state.p1.position.y += penetration;
				state.p1.collidedBottom = true;
				if (j == state.blocks.size()-1) {
					Mix_PlayChannel(-1, someSound, 0);
					mode = STATE_GAME_LEVEL3;
				}
			}
			else if (state.p1.position.y < state.blocks[j].position.y - state.blocks[j].size.y / 2.0f) {
				penetration = fabs((state.blocks[j].position.y - state.p1.position.y) - state.p1.size.y / 2.0f - state.blocks[j].size.y / 2.0f);
				state.p1.position.y -= penetration;
				state.p1.collidedTop = true;
			}
			
		}
		
		
	}

	//top
	float p_x4 = fabs(state.p1.position.x - state.top.position.x) - ((state.p1.size.x + state.top.size.x) / 2.0f);
	float p_y4 = fabs(state.p1.position.y - state.top.position.y) - ((state.p1.size.y + state.top.size.y) / 2.0f);

	if (p_x4 < 0 && p_y4 < 0) {
		if (state.p1.position.y > state.top.position.y + state.top.size.y / 2.0f) {
			penetration = fabs((state.p1.position.y - state.top.position.y) - state.p1.size.y / 2.0f - state.top.size.y / 2.0f);
			state.p1.position.y += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.y < state.top.position.y - state.top.size.y / 2.0f) {
			penetration = fabs((state.top.position.y - state.p1.position.y) - state.p1.size.y / 2.0f - state.top.size.y / 2.0f);
			state.p1.position.y -= penetration;
			state.p1.collidedTop = true;
		}
	}


	//rmid
	float p_x5 = fabs(state.p1.position.x - state.rmid.position.x) - ((state.p1.size.x + state.rmid.size.x) / 2.0f);
	float p_y5 = fabs(state.p1.position.y - state.rmid.position.y) - ((state.p1.size.y + state.rmid.size.y) / 2.0f);
	if (p_x5 < 0 && p_y5 < 0) {
		if (state.p1.position.y > state.rmid.position.y + state.rmid.size.y / 2.0f) {
			penetration = fabs((state.p1.position.y - state.rmid.position.y) - state.p1.size.y / 2.0f - state.rmid.size.y / 2.0f);
			state.p1.position.y += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.y < state.rmid.position.y - state.rmid.size.y / 2.0f) {
			penetration = fabs((state.rmid.position.y - state.p1.position.y) - state.p1.size.y / 2.0f - state.rmid.size.y / 2.0f);
			state.p1.position.y -= penetration;
			state.p1.collidedTop = true;
		}
	}


	//lmid
	float p_x6 = fabs(state.p1.position.x - state.lmid.position.x) - ((state.p1.size.x + state.lmid.size.x) / 2.0f);
	float p_y6 = fabs(state.p1.position.y - state.lmid.position.y) - ((state.p1.size.y + state.lmid.size.y) / 2.0f);

	if (p_x6 < 0 && p_y6 < 0) {
		if (state.p1.position.y > state.lmid.position.y + state.lmid.size.y / 2.0f) {
			penetration = fabs((state.p1.position.y - state.lmid.position.y) - state.p1.size.y / 2.0f - state.lmid.size.y / 2.0f);
			state.p1.position.y += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.y < state.lmid.position.y - state.lmid.size.y / 2.0f) {
			penetration = fabs((state.lmid.position.y - state.p1.position.y) - state.p1.size.y / 2.0f - state.lmid.size.y / 2.0f);
			state.p1.position.y -= penetration;
			state.p1.collidedTop = true;
		}
	}

	//hill
	float p_x7 = fabs(state.p1.position.x - state.hill.position.x) - ((state.p1.size.x + state.hill.size.x) / 2.0f);
	float p_y7 = fabs(state.p1.position.y - state.hill.position.y) - ((state.p1.size.y + state.hill.size.y) / 2.0f);

	if (p_x7 < 0 && p_y7 < 0) {
		if (state.p1.position.y > state.hill.position.y + state.hill.size.y / 2.0f) {
			penetration = fabs((state.p1.position.y - state.hill.position.y) - state.p1.size.y / 2.0f - state.hill.size.y / 2.0f);
			state.p1.position.y += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.y < state.hill.position.y - state.hill.size.y / 2.0f) {
			penetration = fabs((state.hill.position.y - state.p1.position.y) - state.p1.size.y / 2.0f - state.hill.size.y / 2.0f);
			state.p1.position.y -= penetration;
			state.p1.collidedTop = true;
		}
	}




	//Snake knocks player around
	float p_ex = fabs(state.p1.position.x - state.e1.position.x) - ((state.p1.size.x + state.e1.size.x) / 2);
	float p_ey = fabs(state.p1.position.y - state.e1.position.y) - ((state.p1.size.y + state.e1.size.y) / 2);
	if (p_ex < 0 && p_ey < 0) {
		mode = STATE_GAME_OVER;
	}
}

void UpdateGameLevel3(float elapsed, GameState &state) {
	state.p1.acceleration.x = 0.0f;
	ProcessGameLevelInput(elapsed, state);
	float penetration = 0.0f;
	float penetration_e = 0.0f;
	state.p1.velocity.x = lerp(state.p1.velocity.x, 0.0f, elapsed*friction_x);
	state.p1.velocity.x += state.p1.acceleration.x*elapsed;
	state.p1.position.x += state.p1.velocity.x*elapsed;


	state.e1.position.x += state.e1.velocity.x*elapsed;

	if (state.e1.position.x >= state.p1.position.x && state.e1.velocity.x > 0) {
		state.e1.velocity.x *= -1.0f;
	}
	else if (state.e1.position.x < state.p1.position.x && state.e1.velocity.x < 0) {
		state.e1.velocity.x *= -1.0f;
	}
	float p_x2 = fabs(state.p1.position.x - state.top.position.x) - ((state.p1.size.x + state.top.size.x) / 2.0f);
	float p_y2 = fabs(state.p1.position.y - state.top.position.y) - ((state.p1.size.y + state.top.size.y) / 2.0f);

	if (p_x2 < 0 && p_y2 < 0) {
		if (state.p1.position.x > state.top.position.x + state.top.size.x / 2.0f) {
			penetration = fabs((state.p1.position.x - state.top.position.x) - state.p1.size.x / 2.0f - state.top.size.x / 2.0f);
			state.p1.position.x += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.x < state.top.position.x - state.top.size.x / 2.0f) {
			penetration = fabs((state.top.position.x - state.p1.position.x) - state.p1.size.x / 2.0f - state.top.size.x / 2.0f);
			state.p1.position.x -= penetration;
			state.p1.collidedTop = true;
		}
	}
	//blocks x's
	for (int j = 0; j < state.blocks.size(); j++) {
		float p_x13 = fabs(state.p1.position.x - state.blocks[j].position.x) - ((state.p1.size.x + state.blocks[j].size.x) / 2.0f);
		float p_y13 = fabs(state.p1.position.y - state.blocks[j].position.y) - ((state.p1.size.y + state.blocks[j].size.y) / 2.0f);


		if (p_x13 <= 0 && p_y13 <= 0) {
			if (state.p1.position.x > state.blocks[j].position.x + state.blocks[j].size.x / 2.0f) {
				penetration = fabs((state.p1.position.x - state.blocks[j].position.x) - state.p1.size.x / 2.0f - state.blocks[j].size.x / 2.0f);
				state.p1.position.x += penetration;
				state.p1.collidedBottom = true;
			}
			else if (state.p1.position.x < state.blocks[j].position.x - state.blocks[j].size.x / 2.0f) {
				penetration = fabs((state.blocks[j].position.x - state.p1.position.x) - state.p1.size.x / 2.0f - state.blocks[j].size.x / 2.0f);
				state.p1.position.x -= penetration;
				state.p1.collidedTop = true;
			}
		}


	}
	float p_x3 = fabs(state.p1.position.x - state.rmid.position.x) - ((state.p1.size.x + state.rmid.size.x) / 2.0f);
	float p_y3 = fabs(state.p1.position.y - state.rmid.position.y) - ((state.p1.size.y + state.rmid.size.y) / 2.0f);

	if (p_x3 < 0 && p_y3 < 0) {
		if (state.p1.position.x > state.rmid.position.x + state.rmid.size.x / 2.0f) {
			penetration = fabs((state.p1.position.x - state.rmid.position.x) - state.p1.size.x / 2.0f - state.rmid.size.x / 2.0f);
			state.p1.position.x += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.x < state.rmid.position.x - state.rmid.size.x / 2.0f) {
			penetration = fabs((state.rmid.position.x - state.p1.position.x) - state.p1.size.x / 2.0f - state.rmid.size.x / 2.0f);
			state.p1.position.x -= penetration;
			state.p1.collidedTop = true;
		}
	}

	float p_x8 = fabs(state.p1.position.x - state.lmid.position.x) - ((state.p1.size.x + state.lmid.size.x) / 2.0f);
	float p_y8 = fabs(state.p1.position.y - state.lmid.position.y) - ((state.p1.size.y + state.lmid.size.y) / 2.0f);

	if (p_x8 < 0 && p_y8 < 0) {
		if (state.p1.position.x > state.lmid.position.x + state.lmid.size.x / 2.0f) {
			penetration = fabs((state.p1.position.x - state.lmid.position.x) - state.p1.size.x / 2.0f - state.lmid.size.x / 2.0f);
			state.p1.position.x += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.x < state.lmid.position.x - state.lmid.size.x / 2.0f) {
			penetration = fabs((state.lmid.position.x - state.p1.position.x) - state.p1.size.x / 2.0f - state.lmid.size.x / 2.0f);
			state.p1.position.x -= penetration;
			state.p1.collidedTop = true;
		}
	}

	float p_x9 = fabs(state.p1.position.x - state.hill.position.x) - ((state.p1.size.x + state.hill.size.x) / 2.0f);
	float p_y9 = fabs(state.p1.position.y - state.hill.position.y) - ((state.p1.size.y + state.hill.size.y) / 2.0f);
	float e_x9 = fabs(state.e1.position.x - state.hill.position.x) - ((state.e1.size.x + state.hill.size.x) / 2.0f);
	float e_y9 = fabs(state.e1.position.y - state.hill.position.y) - ((state.e1.size.y + state.hill.size.y) / 2.0f);

	if (p_x9 < 0 && p_y9 < 0) {
		if (state.p1.position.x > state.hill.position.x + state.hill.size.x / 2.0f) {
			penetration = fabs((state.p1.position.x - state.hill.position.x) - state.p1.size.x / 2.0f - state.hill.size.x / 2.0f);
			state.p1.position.x += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.x < state.hill.position.x - state.hill.size.x / 2.0f) {
			penetration = fabs((state.hill.position.x - state.p1.position.x) - state.p1.size.x / 2.0f - state.hill.size.x / 2.0f);
			state.p1.position.x -= penetration;
			state.p1.collidedTop = true;
		}
	}
	if (e_x9 < 0 && e_y9 < 0) {
		//collision with bottom of sprite
		if (state.e1.position.x > state.hill.position.x + state.hill.size.x / 2.0f) {
			penetration = fabs((state.e1.position.x - state.hill.position.x) - state.e1.size.x / 2.0f - state.hill.size.x / 2.0f);
			state.e1.position.x += penetration;
			state.e1.collidedBottom = true;
		}
		//collision with top of sprite
		else if (state.e1.position.x < state.hill.position.x - state.hill.size.x / 2.0f) {
			penetration = fabs((state.hill.position.x - state.e1.position.x) - state.e1.size.x / 2.0f - state.hill.size.x / 2.0f);
			state.e1.position.x -= penetration;
			state.e1.collidedTop = true;
		}
	}

	state.p1.acceleration.y = 0.0f;
	state.p1.acceleration.y = -1.5f;
	state.p1.velocity.y = lerp(state.p1.velocity.y, 0.0f, elapsed*friction_y);
	state.p1.velocity.y += state.p1.acceleration.y*elapsed;
	state.p1.position.y += state.p1.velocity.y*elapsed;


	state.p1.collidedBottom = false;
	state.p1.collidedTop = false;
	state.p1.collidedRight = false;
	state.p1.collidedLeft = false;
	//enemy Movement
	state.e1.acceleration.y = 0.0f;
	state.e1.acceleration.y = -1.5f;
	state.e1.velocity.y = lerp(state.e1.velocity.y, 0.0f, elapsed*friction_y);
	state.e1.velocity.y += state.e1.acceleration.y*elapsed;
	state.e1.position.y += state.e1.velocity.y*elapsed;



	state.e1.collidedBottom = false;
	state.e1.collidedTop = false;
	state.e1.collidedRight = false;
	state.e1.collidedLeft = false;


	//floor
	for (int i = 0; i < state.floors.size(); i++) {
		float p_x = fabs(state.p1.position.x - state.floors[i].position.x) - ((state.p1.size.x + state.floors[i].size.x) / 2.0f);
		float p_y = fabs(state.p1.position.y - state.floors[i].position.y) - ((state.p1.size.y + state.floors[i].size.y) / 2.0f);
		float e_x = fabs(state.e1.position.x - state.floors[i].position.x) - ((state.e1.size.x + state.floors[i].size.x) / 2.0f);
		float e_y = fabs(state.e1.position.y - state.floors[i].position.y) - ((state.e1.size.y + state.floors[i].size.y) / 2.0f);

		if (p_x <= 0 && p_y <= 0) {
			penetration = fabs((state.p1.position.y - state.floors[i].position.y) - state.p1.size.y / 2.0f - state.floors[i].size.y / 2.0f);
			state.p1.position.y += penetration;
			state.p1.collidedBottom = true;
		}
		if (e_x <= 0 && e_y <= 0) {
			penetration_e = fabs((state.e1.position.y - state.floors[i].position.y) - state.e1.size.y / 2.0f - state.floors[i].size.y / 2.0f);
			state.e1.position.y += penetration_e;
			state.e1.collidedBottom = true;
		}
	}

	//blocks
	for (int j = 0; j < state.blocks.size(); j++) {
		float p_x12 = fabs(state.p1.position.x - state.blocks[j].position.x) - ((state.p1.size.x + state.blocks[j].size.x) / 2.0f);
		float p_y12 = fabs(state.p1.position.y - state.blocks[j].position.y) - ((state.p1.size.y + state.blocks[j].size.y) / 2.0f);


		if (p_x12 <= 0 && p_y12 <= 0) {
			if (state.p1.position.y > state.blocks[j].position.y + state.blocks[j].size.y / 2.0f) {
				penetration = fabs((state.p1.position.y - state.blocks[j].position.y) - state.p1.size.y / 2.0f - state.blocks[j].size.y / 2.0f);
				state.p1.position.y += penetration;
				state.p1.collidedBottom = true;
				if (j == state.blocks.size() - 1) {
					Mix_PlayChannel(-1, someSound, 0);
					mode = STATE_GAME_WIN;
				}
			}
			else if (state.p1.position.y < state.blocks[j].position.y - state.blocks[j].size.y / 2.0f) {
				penetration = fabs((state.blocks[j].position.y - state.p1.position.y) - state.p1.size.y / 2.0f - state.blocks[j].size.y / 2.0f);
				state.p1.position.y -= penetration;
				state.p1.collidedTop = true;
			}

		}


	}

	//top
	float p_x4 = fabs(state.p1.position.x - state.top.position.x) - ((state.p1.size.x + state.top.size.x) / 2.0f);
	float p_y4 = fabs(state.p1.position.y - state.top.position.y) - ((state.p1.size.y + state.top.size.y) / 2.0f);

	if (p_x4 < 0 && p_y4 < 0) {
		if (state.p1.position.y > state.top.position.y + state.top.size.y / 2.0f) {
			penetration = fabs((state.p1.position.y - state.top.position.y) - state.p1.size.y / 2.0f - state.top.size.y / 2.0f);
			state.p1.position.y += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.y < state.top.position.y - state.top.size.y / 2.0f) {
			penetration = fabs((state.top.position.y - state.p1.position.y) - state.p1.size.y / 2.0f - state.top.size.y / 2.0f);
			state.p1.position.y -= penetration;
			state.p1.collidedTop = true;
		}
	}


	//rmid
	float p_x5 = fabs(state.p1.position.x - state.rmid.position.x) - ((state.p1.size.x + state.rmid.size.x) / 2.0f);
	float p_y5 = fabs(state.p1.position.y - state.rmid.position.y) - ((state.p1.size.y + state.rmid.size.y) / 2.0f);
	if (p_x5 < 0 && p_y5 < 0) {
		if (state.p1.position.y > state.rmid.position.y + state.rmid.size.y / 2.0f) {
			penetration = fabs((state.p1.position.y - state.rmid.position.y) - state.p1.size.y / 2.0f - state.rmid.size.y / 2.0f);
			state.p1.position.y += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.y < state.rmid.position.y - state.rmid.size.y / 2.0f) {
			penetration = fabs((state.rmid.position.y - state.p1.position.y) - state.p1.size.y / 2.0f - state.rmid.size.y / 2.0f);
			state.p1.position.y -= penetration;
			state.p1.collidedTop = true;
		}
	}


	//lmid
	float p_x6 = fabs(state.p1.position.x - state.lmid.position.x) - ((state.p1.size.x + state.lmid.size.x) / 2.0f);
	float p_y6 = fabs(state.p1.position.y - state.lmid.position.y) - ((state.p1.size.y + state.lmid.size.y) / 2.0f);

	if (p_x6 < 0 && p_y6 < 0) {
		if (state.p1.position.y > state.lmid.position.y + state.lmid.size.y / 2.0f) {
			penetration = fabs((state.p1.position.y - state.lmid.position.y) - state.p1.size.y / 2.0f - state.lmid.size.y / 2.0f);
			state.p1.position.y += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.y < state.lmid.position.y - state.lmid.size.y / 2.0f) {
			penetration = fabs((state.lmid.position.y - state.p1.position.y) - state.p1.size.y / 2.0f - state.lmid.size.y / 2.0f);
			state.p1.position.y -= penetration;
			state.p1.collidedTop = true;
		}
	}

	//hill
	float p_x7 = fabs(state.p1.position.x - state.hill.position.x) - ((state.p1.size.x + state.hill.size.x) / 2.0f);
	float p_y7 = fabs(state.p1.position.y - state.hill.position.y) - ((state.p1.size.y + state.hill.size.y) / 2.0f);

	if (p_x7 < 0 && p_y7 < 0) {
		if (state.p1.position.y > state.hill.position.y + state.hill.size.y / 2.0f) {
			penetration = fabs((state.p1.position.y - state.hill.position.y) - state.p1.size.y / 2.0f - state.hill.size.y / 2.0f);
			state.p1.position.y += penetration;
			state.p1.collidedBottom = true;
		}
		else if (state.p1.position.y < state.hill.position.y - state.hill.size.y / 2.0f) {
			penetration = fabs((state.hill.position.y - state.p1.position.y) - state.p1.size.y / 2.0f - state.hill.size.y / 2.0f);
			state.p1.position.y -= penetration;
			state.p1.collidedTop = true;
		}
	}




	//Snake knocks player around
	float p_ex = fabs(state.p1.position.x - state.e1.position.x) - ((state.p1.size.x + state.e1.size.x) / 2);
	float p_ey = fabs(state.p1.position.y - state.e1.position.y) - ((state.p1.size.y + state.e1.size.y) / 2);
	if (p_ex < 0 && p_ey < 0) {
		mode = STATE_GAME_OVER;
	}
}

void UpdateGameOver(float elapsed) {

}

void ProcessMainMenuInput() {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_Y]) {
		mode = STATE_GAME_LEVEL;
	}
}

void ProcessGameOverInput() {
	
}

void ProcessGameWinInput() {
	
}


void Render(ShaderProgram &program) {
	switch (mode) {
	case STATE_MAIN_MENU:
		RenderMainMenu(program);
		break;
	case STATE_GAME_LEVEL:
		RenderGameLevel(program);
		break;
	case STATE_GAME_LEVEL2:
		RenderGameLevel2(program);
		break;
	case STATE_GAME_LEVEL3:
		RenderGameLevel3(program);
		break;
	case STATE_GAME_OVER:
		RenderGameOver(program);
		break;
	case STATE_GAME_WIN:
		RenderGameWin(program);
		break;
	}
	
}
void Update(float elapsed) {
	switch (mode) {
	case STATE_MAIN_MENU:
		UpdateMainMenu(elapsed);
		break;
	case STATE_GAME_LEVEL:
		UpdateGameLevel(elapsed, state1);
		break;
	case STATE_GAME_LEVEL2:
		UpdateGameLevel2(elapsed, state2);
		break;
	case STATE_GAME_LEVEL3:
		UpdateGameLevel3(elapsed, state3);
		break;
	case STATE_GAME_OVER:
		UpdateGameOver(elapsed);
	case STATE_GAME_WIN:
		UpdateGameWin(elapsed);
	}
}

void ProcessInput(float elapsed) {
	switch (mode) {
	case STATE_MAIN_MENU:
		ProcessMainMenuInput();
		break;
	case STATE_GAME_LEVEL:
		break;
	case STATE_GAME_LEVEL2:
		break;
	case STATE_GAME_LEVEL3:
		break;
	case STATE_GAME_OVER:
		ProcessGameOverInput();
		break;
	case STATE_GAME_WIN:
		ProcessGameWinInput();
		break;
	}
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	//added
	glViewport(0, 0, 640, 360);
	ShaderProgram program;
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	const int runAnimation[] = { 0,1,2,3 };
	const int numFrames = 4;
	float animationElapsed = 0.0f;
	float framesPerSecond = 60.0f;
	int currentIndex = 0;

	characters = LoadTexture(RESOURCE_FOLDER"characters_3.png");
	SheetSprite player = SheetSprite(characters, currentIndex, 8, 4);
	player.size = 0.2f;
	SheetSprite enemy = SheetSprite(characters, 26, 8, 4);
	enemy.size = 0.2f;
	Letters = LoadTexture(RESOURCE_FOLDER"font2.png");
	glm::mat4 projectionMatrix = glm::mat4(1.0f);

	World = LoadTexture(RESOURCE_FOLDER"arne_sprites.png");
	SheetSprite earth = SheetSprite(World, 1, 16, 8);
	earth.size = 0.3f;

	//p1
	Entity p1;
	p1.sprite = player;
	p1.position.x = 0.0f;
	p1.position.y = -0.60f;
	p1.velocity.x = 0.0f;
	p1.velocity.y = -0.0f;
	p1.acceleration.x = 0.0f;
	p1.acceleration.y = 0.0f;
	p1.isStatic = false;
	p1.size = glm::vec3(player.size, player.size, 0.0f);
	state1.p1 = p1;
	state2.p1 = p1;
	state3.p1 = p1;

	//e1
	Entity e1;
	e1.sprite = enemy;
	e1.position.x = 1.0f;
	e1.position.y = -0.61f;
	e1.velocity.y = 0.0f;
	e1.velocity.x = 0.4f;
	e1.size = glm::vec3(enemy.size, enemy.size, 0.0f);
	e1.isStatic = false;

	state1.e1 = e1;
	state2.e1 = e1;
	state3.e1 = e1;

	//Ground
	std::vector<Entity> floors;
	for (int i = 0; i <= 12; i++) {
		Entity floor;
		floor.sprite = earth;
		floor.isStatic = true;
		floor.size = glm::vec3(earth.size, earth.size, 0.0f);
		floors.push_back(floor);
	}

	float x_offset = -1.8f;
	float y_offset = -0.87f;
	for (int i = 0; i < floors.size(); i++) {

		floors[i].position.x = x_offset;
		floors[i].position.y = y_offset;
		x_offset += floors[i].size.x;

	}
	state1.floors = floors;
	state2.floors = floors;
	state3.floors = floors;

	earth.size = 0.2f;
	std::vector<Entity> blocks2;
	for (int i = 0; i <= 4; i++) {
		Entity block;
		block.sprite = earth;
		block.isStatic = true;
		block.size = glm::vec3(earth.size, earth.size, 0.0f);
		blocks2.push_back(block);
	}
	float x_offset2 = 0.5f;
	float y_offset2 = 1.0f;
	for (int i = 0; i < blocks2.size(); i++) {

		blocks2[i].position.x = x_offset2;
		blocks2[i].position.y = y_offset2;
		x_offset2 += blocks2[i].size.x;
		y_offset2 += 0.2f;
	}
	state2.blocks = blocks2;

	std::vector<Entity> blocks3;
	for (int i = 0; i <= 10; i++) {
		Entity block;
		block.sprite = earth;
		block.isStatic = true;
		block.size = glm::vec3(earth.size, earth.size, 0.0f);
		blocks3.push_back(block);
	}
	float x_offset3 = 1.7f;
	float y_offset3 = 0.6f;
	for (int i = 0; i < blocks3.size(); i++) {
		if (i < 4) {
			blocks3[i].position.x = x_offset3;
			blocks3[i].position.y = y_offset3;
			x_offset3 += blocks2[i].size.x;
			y_offset3 += 0.2f;
		}
		else if (i >= 4 && i <= 7) {
			x_offset3 -= 1.0f;
			y_offset3 += 0.4f;
			blocks3[i].position.x = x_offset3;
			blocks3[i].position.y = y_offset3;
		}
		else {
			x_offset3 += 1.3f;
			y_offset3 += 0.5f;
			blocks3[i].position.x = x_offset3;
			blocks3[i].position.y = y_offset3;
		}
	}
	state3.blocks = blocks3;

	earth.size = 0.2f;
	Entity rmid;
	rmid.sprite = earth;
	rmid.position.x = 1.0f;
	rmid.position.y = -0.2f;
	rmid.isStatic = true;
	rmid.size = glm::vec3(earth.size, earth.size, 0.0f);
	state1.rmid = rmid;
	state2.rmid = rmid;
	state3.rmid = rmid;

	Entity lmid;
	lmid.sprite = earth;
	lmid.position.x = -1.0f;
	lmid.position.y = -0.2f;
	lmid.isStatic = true;
	lmid.size = glm::vec3(earth.size, earth.size, 0.0f);
	state1.lmid = lmid;
	state2.lmid = lmid;
	state3.lmid = lmid;

	Entity hill;
	hill.sprite = earth;
	hill.position.x = -0.3f;
	hill.position.y = -0.62f;
	hill.isStatic = true;
	hill.size = glm::vec3(earth.size, earth.size, 0.0f);
	state1.hill = hill;
	state2.hill = hill;
	state3.hill = hill;

	//top
	Entity top;
	top.sprite = earth;
	top.position.x = 0.0f;
	top.position.y = 0.3f;
	top.isStatic = true;
	top.size = glm::vec3(earth.size, earth.size, 0.0f);
	state1.top = top;
	state2.top = top;
	state3.top = top;

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	
	someSound = Mix_LoadWAV("funny-yay.wav"); 
	Mix_VolumeChunk(someSound, 30);
	
	Mix_Music* music;
	music = Mix_LoadMUS("Mii Plaza.mp3");
	Mix_VolumeMusic(30);
	Mix_PlayMusic(music, -1);
	
	//Letters
	glm::mat4 modelMatrix = glm::mat4(1.0f);




	glm::mat4 viewMatrix = glm::mat4(1.0f);





	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);
	glUseProgram(program.programID);

	float lastFrameTicks = 0.0f;
	float angle = 0.0f;



	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	SDL_Event event;
	bool done = false;

	float accumulator = 0.0f;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {

				}
			}
		}

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		elapsed += accumulator;
		if (elapsed < FIXED_TIMESTEP) {
			accumulator = elapsed;
			continue;
		}
		while (elapsed >= FIXED_TIMESTEP) {
			Update(FIXED_TIMESTEP);
			elapsed -= FIXED_TIMESTEP;
		}
		accumulator = elapsed;
		glClear(GL_COLOR_BUFFER_BIT);
		if (mode == STATE_GAME_LEVEL) {
			viewMatrix = glm::mat4(1.0f);
			viewMatrix = glm::translate(viewMatrix, glm::vec3(-state1.p1.position.x, -state1.p1.position.y, 0.0f));
			program.SetViewMatrix(viewMatrix);
		}
		else if (mode == STATE_GAME_LEVEL2) {
			viewMatrix = glm::mat4(1.0f);
			viewMatrix = glm::translate(viewMatrix, glm::vec3(-state2.p1.position.x, -state2.p1.position.y, 0.0f));
			program.SetViewMatrix(viewMatrix);
		}
		else if (mode == STATE_GAME_LEVEL3) {
			viewMatrix = glm::mat4(1.0f);
			viewMatrix = glm::translate(viewMatrix, glm::vec3(-state3.p1.position.x, -state3.p1.position.y, 0.0f));
			program.SetViewMatrix(viewMatrix);
		}
		else {
			viewMatrix = glm::mat4(1.0f);
			program.SetViewMatrix(viewMatrix);
		}

		animationElapsed += elapsed;
		if (animationElapsed > 1.0 / framesPerSecond) {
			currentIndex++;
			animationElapsed = 0.0;
			if (currentIndex > numFrames - 1) {
				currentIndex = 0;
			}
		}
		state1.p1.sprite = SheetSprite(characters,runAnimation[currentIndex], 8, 4);
		state1.p1.sprite.size = 0.2f;

		state2.p1.sprite = SheetSprite(characters, runAnimation[currentIndex], 8, 4);
		state2.p1.sprite.size = 0.2f;

		state3.p1.sprite = SheetSprite(characters, runAnimation[currentIndex], 8, 4);
		state3.p1.sprite.size = 0.2f;

		
		if (mode == STATE_GAME_OVER) {
			Mix_HaltMusic();
		}
		Render(program);
		Update(elapsed);
		ProcessInput(elapsed);

		SDL_GL_SwapWindow(displayWindow);
	}

	

	SDL_Quit();
	
	return 0;
}
