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


#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	float ball_x_speed = .1;
	float ball_y_speed = .1;
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
	

	program.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);



	glm::mat4 projectionMatrix = glm::mat4(1.0f);

	glm::mat4 modelMatrix = glm::mat4(1.0f);


	glm::mat4 modelMatrix2 = glm::mat4(1.0f);
	modelMatrix2 = glm::translate(modelMatrix2, glm::vec3(1.5f, 0.0f, 0.0f));
	modelMatrix2 = glm::scale(modelMatrix2, glm::vec3(0.05f, 0.2f, 0.0f));
	glm::mat4 modelMatrix3 = glm::mat4(1.0f);
	

	glm::mat4 viewMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);
	glUseProgram(program.programID);

	float lastFrameTicks = 0.0f;
	float angle = 0.0f;

	float left_paddle_x = -1.5f;
	float left_paddle_y = 0.0f;

	float right_paddle_x = 1.5f;
	float right_paddle_y = 0.0f;

	float paddle_width = 1.0f;
	float paddle_height = 1.0f;

	float ball_x = 0.0f;
	float ball_y = 0.0f;

	float ball_dir_x = 1.0f;
	float ball_dir_y = 1.0f;

	float ball_width = 1.0f;
	float ball_height = 1.0f;

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
						
		}

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		glClear(GL_COLOR_BUFFER_BIT);

		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		if (keys[SDL_SCANCODE_UP] && right_paddle_y + (0.5*paddle_height) < 1.0f) {
			right_paddle_y += elapsed;
		}
		else if (keys[SDL_SCANCODE_DOWN] && right_paddle_y - (0.5*paddle_height) > -1.0f) {
			right_paddle_y -= elapsed;
		}
		if (keys[SDL_SCANCODE_W] && left_paddle_y+(0.5*paddle_height) < 1.0f) {
			left_paddle_y += elapsed;
		}
		else if (keys[SDL_SCANCODE_S] && left_paddle_y-(0.5*paddle_height) > -1.0f) {
			left_paddle_y -= elapsed;
		}

		//right paddle ball equation
		float p = fabs(ball_x - right_paddle_x) - ((ball_width + paddle_width) / 2);
		float p2 = fabs(ball_y - right_paddle_y) - ((ball_height + paddle_height) / 2);

		//left paddle ball equation
		float p3 = fabs(ball_x - left_paddle_x) - ((ball_width + paddle_width) / 2);
		float p4 = fabs(ball_y - left_paddle_y) - ((ball_height + paddle_height) / 2);

		if (p < 0 && p2 < 0) {
			ball_dir_x *= -1;
		}

		if (p3 < 0 && p4 < 0) {
			ball_dir_x *= -1;
		}

		//ball top collision
		if (ball_y + (ball_height*0.5) >= 1.0f) {
			ball_dir_y *= -1;
		}
		//ball bottom collision
		else if (ball_y - (ball_height*0.5) <= -1.0f) {
			ball_dir_y *= -1;
		}
		//ball side collision
		if (ball_x + (ball_width*0.5) >= 1.777f) {
			//left won
			glClearColor(0.0f, 0.0f, 0.8f, 1.0f);
			ball_x = 0.0f;
			ball_y = 0.0f;
		}
		else if (ball_x - (ball_width*0.5) <= -1.777f) {
			//right won
			glClearColor(0.8f, 0.0f, 0.0f, 1.0f);
			ball_x = 0.0f;
			ball_y = 0.0f;
		}
		ball_x += elapsed*ball_dir_x;
		ball_y += elapsed*ball_dir_y;




		//Left Paddle

		float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);
		paddle_width = 1.0f;
		paddle_height = 1.0f;
		program.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(left_paddle_x, left_paddle_y, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.05f, 0.2f, 0.0f));
		paddle_width *= 0.05f;
		paddle_height *= 0.2f;
		program.SetModelMatrix(modelMatrix);

		
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);

		//Right Paddle
		modelMatrix2 = glm::mat4(1.0f);
		modelMatrix2 = glm::translate(modelMatrix2, glm::vec3(right_paddle_x, right_paddle_y, 0.0f));
		modelMatrix2 = glm::scale(modelMatrix2, glm::vec3(0.05f, 0.2f, 0.0f));
		program.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		program.SetModelMatrix(modelMatrix2);

		
		float vertices2[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices2);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);


		//Ball
		float vertices3[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices3);
		glEnableVertexAttribArray(program.positionAttribute);
		ball_height = 1.0f;
		ball_width = 1.0f;
		program.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		modelMatrix3 = glm::mat4(1.0f);
		modelMatrix3 = glm::translate(modelMatrix3, glm::vec3(ball_x, ball_y, 0.0f));
		modelMatrix3 = glm::scale(modelMatrix3, glm::vec3(0.05f, 0.05f, 0.0f));
		ball_height *= 0.05f;
		ball_width *= 0.05f;
		program.SetModelMatrix(modelMatrix3);

		
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);


		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
