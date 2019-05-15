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
	ShaderProgram Itachi;
	ShaderProgram program;
	GLuint ita = LoadTexture(RESOURCE_FOLDER"Itachi.png");
	GLuint amat = LoadTexture(RESOURCE_FOLDER"Amaterasu.png");
	GLuint rain = LoadTexture(RESOURCE_FOLDER"Rain.jpg");

	program.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	Itachi.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	Itachi.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	

	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	glm::mat4 projectionMatrix2 = glm::mat4(1.0f);
	glm::mat4 projectionMatrix3 = glm::mat4(1.0f);
	glm::mat4 projectionMatrix4 = glm::mat4(1.0f);

	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -1.0f, 0.0f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(4.0f, 1.0f, 1.0f));
	glm::mat4 modelMatrix2 = glm::mat4(1.0f);
	modelMatrix2 = glm::translate(modelMatrix2, glm::vec3(-1.0f, 0.0f, 0.0f));
	glm::mat4 modelMatrix3 = glm::mat4(1.0f);
	modelMatrix3 = glm::translate(modelMatrix3, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 modelMatrix4 = glm::mat4(1.0f);
	modelMatrix4 = glm::scale(modelMatrix4, glm::vec3(4.0f, 2.0f, 1.0f));
	
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	glm::mat4 viewMatrix2 = glm::mat4(1.0f);
	glm::mat4 viewMatrix3 = glm::mat4(1.0f);
	glm::mat4 viewMatrix4 = glm::mat4(1.0f);

	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	projectionMatrix2 = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	projectionMatrix3 = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	projectionMatrix4 = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);

	glUseProgram(program.programID);
	glUseProgram(Itachi.programID);

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);
		//added

		//Rain Background Image
		Itachi.SetModelMatrix(modelMatrix4);
		Itachi.SetProjectionMatrix(projectionMatrix4);
		Itachi.SetViewMatrix(viewMatrix4);

		glBindTexture(GL_TEXTURE_2D, rain);


		float vertices3[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(Itachi.positionAttribute, 2, GL_FLOAT, false, 0, vertices3);
		glEnableVertexAttribArray(Itachi.positionAttribute);

		float texCoords3[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(Itachi.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords3);
		glEnableVertexAttribArray(Itachi.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(Itachi.positionAttribute);
		glDisableVertexAttribArray(Itachi.texCoordAttribute);

		
		//Ground
		program.SetColor(0.0f, 0.2f, 0.0f, 1.0f);
		program.SetModelMatrix(modelMatrix);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);
		
		float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);         
		glDrawArrays(GL_TRIANGLES, 0, 6);         
		glDisableVertexAttribArray(program.positionAttribute);

		//Itachi Image
		Itachi.SetModelMatrix(modelMatrix2);
		Itachi.SetProjectionMatrix(projectionMatrix2);
		Itachi.SetViewMatrix(viewMatrix2);

		glBindTexture(GL_TEXTURE_2D, ita);

		
		float vertices1[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(Itachi.positionAttribute, 2, GL_FLOAT, false, 0, vertices1);
		glEnableVertexAttribArray(Itachi.positionAttribute);

		float texCoords1[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(Itachi.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords1);
		glEnableVertexAttribArray(Itachi.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisableVertexAttribArray(Itachi.positionAttribute);
		glDisableVertexAttribArray(Itachi.texCoordAttribute);


		//Amaterasu Image
		Itachi.SetModelMatrix(modelMatrix3);
		Itachi.SetProjectionMatrix(projectionMatrix3);
		Itachi.SetViewMatrix(viewMatrix3);

		glBindTexture(GL_TEXTURE_2D, amat);


		float vertices2[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(Itachi.positionAttribute, 2, GL_FLOAT, false, 0, vertices2);
		glEnableVertexAttribArray(Itachi.positionAttribute);

		float texCoords2[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(Itachi.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords2);
		glEnableVertexAttribArray(Itachi.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(Itachi.positionAttribute);
		glDisableVertexAttribArray(Itachi.texCoordAttribute);
		

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
