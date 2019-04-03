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
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <vector>
#define MAX_BULLETS 30

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;
enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL };
GameMode mode = STATE_MAIN_MENU;

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

GLuint sheet;
GLuint Letters;
class SheetSprite {
public:
	SheetSprite() {}
	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size) : textureID(textureID), u(u), v(v), width(width), height(height), size(size) {}
	void Draw(ShaderProgram &program);
	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};

void SheetSprite::Draw(ShaderProgram &program) {
	glBindTexture(GL_TEXTURE_2D, textureID);
	GLfloat texCoords[] = { u, v + height,
		u + width, v,
		u, v,
		u + width, v,
		u, v + height,
		u + width, v + height
	};
	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);
	float aspect = width / height;
	float vertices[] = {
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, 0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, -0.5f * size,
	};
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);

}



class Entity {
public:
	void Draw(ShaderProgram &program) {

		//Players Ship
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		glBindTexture(GL_TEXTURE_2D, sheet);
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(position.x, position.y, 0.0f));
		program.SetModelMatrix(modelMatrix);

		sprite.Draw(program);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

	};

	void setSize(float x, float y) {
		size.x = x;
		size.y = y;
	}

	glm::vec3 position;
	glm::vec3 size;
	glm::vec3 velocity;

	SheetSprite sprite;
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


void Shoot(Entity& bullet, float& x, float& y) {
	bullet.position.x = x;
	bullet.position.y = y;
}

class GameState {
public:
	Entity ship;
	std::vector<Entity> bullets;
	std::vector<Entity> invaders;

};
GameState state;

void RenderMainMenu(ShaderProgram &program) {
	DrawText(program,Letters, "Space Invaders", 0.2f, 0.0f, -1.3f, 0.0f);
	DrawText(program, Letters, "(Press y to start)", 0.1f, 0.0f, -0.8f, -0.3f);
}

void RenderGameLevel(ShaderProgram &program) {
	//Space Ship
	state.ship.Draw(program);

	//Bullets
	for (int i = 0; i < MAX_BULLETS; i++) {
		state.bullets[i].Draw(program);
	}

	//Invaders
	for (int i = 0; i < state.invaders.size(); i++) {
		state.invaders[i].Draw(program);

	}

}

void UpdateMainMenu(float elapsed) {

}

void UpdateGameLevel(float elapsed) {
	//Bullets
	for (int i = 0; i < MAX_BULLETS; i++) {
		state.bullets[i].position.y += elapsed;
	}

	for (int j = 0; j < state.invaders.size(); j++) {
		state.invaders[j].position.x += elapsed * state.invaders[j].velocity.x;
	}

	for (int i = 0; i < state.bullets.size(); i++) {
		for (int z = 0; z < state.invaders.size(); z++) {
			float p = fabs(state.bullets[i].position.x - state.invaders[z].position.x) - ((state.bullets[i].size.x + state.invaders[z].size.x) / 2);
			float p2 = fabs(state.bullets[i].position.y - state.invaders[z].position.y) - ((state.bullets[i].size.y + state.invaders[z].size.y) / 2);


			if (p < 0 && p2 < 0) {
				state.invaders[z].position.x = -2000.0f;
				state.bullets[i].position.x = -1000.0f;
			}
		}
	}
	
}

void ProcessMainMenuInput() {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_Y]) {
		mode = STATE_GAME_LEVEL;
	}
}

void ProcessGameLevelInput(float elapsed) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_RIGHT] && state.ship.position.x + (0.5*state.ship.size.x) < 1.777f) {
		state.ship.position.x += elapsed;
	}
	else if (keys[SDL_SCANCODE_LEFT] && state.ship.position.x - (0.5*state.ship.size.x) > -1.777f) {
		state.ship.position.x -= elapsed;
	}
	//Invaders sides collisions
	for (int z = 0; z < state.invaders.size(); z++)
		if (state.invaders[z].position.x + (state.invaders[z].size.x*0.5) >= 1.777f) {
			state.invaders[z].velocity.x *= -1;
		}
		else if (state.invaders[z].position.x - (state.invaders[z].size.x*0.5) <= -1.777f) {
			state.invaders[z].velocity.x *= -1;
		}


}

void Render(ShaderProgram &program) {
	switch (mode) {
	case STATE_MAIN_MENU:
		RenderMainMenu(program);
		break;
	case STATE_GAME_LEVEL:
		RenderGameLevel(program);
		break;
	}
}
void Update(float elapsed) {
	switch (mode) {
	case STATE_MAIN_MENU:
		UpdateMainMenu(elapsed);
		break;
	case STATE_GAME_LEVEL:
		UpdateGameLevel(elapsed);
		break;
	}
}

void ProcessInput(float elapsed) {
	switch (mode) {
	case STATE_MAIN_MENU:
		ProcessMainMenuInput();
		break;
	case STATE_GAME_LEVEL:
		ProcessGameLevelInput(elapsed);
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


	sheet = LoadTexture(RESOURCE_FOLDER"sheet.png");
	SheetSprite ship_skin = SheetSprite(sheet, 465.0f / 1024.0f, 991.0f / 1024.0f, 37.0f / 1024.0f, 26.0f / 1024.0f, 0.2f);
	Letters = LoadTexture(RESOURCE_FOLDER"font2.png");
	glm::mat4 projectionMatrix = glm::mat4(1.0f);


	//Ship
	Entity ship;
	ship.sprite = ship_skin;
	ship.position.x = 0.0f;
	ship.position.y = -0.8f;
	ship.setSize(ship_skin.size*(ship_skin.width / ship_skin.height), ship_skin.size);
	state.ship = ship;

	//Letters
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	

	//Invaders
	SheetSprite mySprite = SheetSprite(sheet, 425.0f / 1024.0f, 468.0f / 1024.0f, 93.0f / 1024.0f, 84.0f / 1024.0f, 0.2f);

	std::vector<Entity> invaders;
	for (int i = 0; i < 30; i++) {
		Entity invader;
		invader.setSize(mySprite.size*(mySprite.width / mySprite.height), mySprite.size);
		invader.position.x = -1.0f;
		invader.position.y = 0.8f;
		invader.velocity.x = 1.0f;
		invader.sprite = mySprite;
		invaders.push_back(invader);
	}
	//Invaders
	float x_offset = -1.0f;
	float y_offset = 0.8f;
	for (int i = 0; i < invaders.size(); i++) {

		invaders[i].position.x = x_offset;
		invaders[i].position.y = y_offset;
		invaders[i].velocity.x = 1.0f;
		x_offset += invaders[i].size.x;
		if (i % 10 == 0 && i != 0) {
			x_offset = -1.0f;
			y_offset -= invaders[i].size.y;
		}


	}
	state.invaders = invaders;

	//Bullets

	int bullind = 0;
	std::vector<Entity> bullets;
	SheetSprite bull_sprite = SheetSprite(sheet, 841.0f / 1024.0f, 647.0f / 1024.0f, 13.0f / 1024.0f, 37.0f / 1024.0f, 0.05f);
	for (int i = 0; i < MAX_BULLETS; i++) {
		Entity bullet;
		bullet.size.x = 1.0f;
		bullet.size.y = 1.0f;
		bullet.sprite = bull_sprite;
		bullet.setSize(bull_sprite.size *(bull_sprite.width / bull_sprite.height), bull_sprite.size);
		bullets.push_back(bullet);
	}
	//bullet pool
	for (int i = 0; i < MAX_BULLETS; i++) {
		bullets[i].position.x = -2000.0f;
	}

	state.bullets = bullets;

	
	


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
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					Shoot(state.bullets[bullind], state.ship.position.x, state.ship.position.y);
					if (bullind == MAX_BULLETS - 1) {
						bullind = 0;
					}
					else {
						bullind += 1;
					}
				}
			}
		}

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		glClear(GL_COLOR_BUFFER_BIT);

		

		Render(program);
		Update(elapsed);
		ProcessInput(elapsed);

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
