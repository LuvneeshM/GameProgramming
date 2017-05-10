//Luvneesh Mugrai
//Intro to Game Programming

#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "Matrix.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

#define FIXED_TIMESTEP 0.01666666f
#define MAX_TIMESTEPS 6	

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
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

		GLuint logo = LoadTexture(RESOURCE_FOLDER"logo.png");

		float x = 0.0f;
		float y = 0.0f;
		float speed = .20f;
		

		glViewport(0, 0, 640, 360);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
		Matrix projectionMatrix;
		Matrix modelMatrix;
		Matrix viewMatrix;
		projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
		glUseProgram(program.programID);

		float lastTick = 0.0f;

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastTick;
		lastTick = ticks;
		
		float fixedElapsed = elapsed;

		glClear(GL_COLOR_BUFFER_BIT);
		if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
			fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
		}
		while (fixedElapsed >= FIXED_TIMESTEP) {
			fixedElapsed -= FIXED_TIMESTEP;
			if (x < -0.4) {
				speed *= -1;
			}
			else if (x >= 0.4) {
				speed *= -1;
			}
		}
		if (x < -0.4) {
			speed *= -1;
		}
		else if (x >= 0.4) {
			speed *= -1;
		}
		
		x += speed *elapsed;
		y -= speed * elapsed;
		
		//center spinning ball thing
		glBindTexture(GL_TEXTURE_2D, logo);
		float ballVertices[] = { -1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, ballVertices);
		glEnableVertexAttribArray(program.positionAttribute);
		//show all of image
		float texCoords2[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords2);
		glEnableVertexAttribArray(program.texCoordAttribute);
		modelMatrix.identity();
		modelMatrix.Translate(x, y, 1.0f);
		modelMatrix.Scale(1.5*1.4f, 1.5*1.0f, 1.0f);
		//the updates onto the program
		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);
		//draw it
		glDrawArrays(GL_TRIANGLES, 0, 6);

	
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		
		SDL_GL_SwapWindow(displayWindow);
	}


	SDL_Quit();
	return 0;
}
