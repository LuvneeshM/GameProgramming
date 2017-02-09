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

		GLuint movingBall = LoadTexture(RESOURCE_FOLDER"Ball10.png");
		GLuint middleBall = LoadTexture(RESOURCE_FOLDER"reticule.png");
		GLuint ulBase = LoadTexture(RESOURCE_FOLDER"arrow.png");

		float movingBallx = 0;
		float movingBally = 0;
		float speed = 10.0f;
		bool down = false, left = false, right = false; 
		bool up = true;

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
		float angle = 0.0f;

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

		angle += 120.0f * elapsed;
		
		glClear(GL_COLOR_BUFFER_BIT);
		//upper left base
		glBindTexture(GL_TEXTURE_2D, ulBase);
		//set the vertices of image
		//{ Lower Tri (x1,y1 Lower Left),(x2,y2 Lower Right),(x3,y3 Upper Right) | Upper Tri (x1,y1 Lower Left), (x1,y1 Upper Right), (x1, y1 Upper Left) }
		float ulBaseV[] = { -1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, ulBaseV);
		glEnableVertexAttribArray(program.positionAttribute);
		//show all of image
		float ulBaseTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, ulBaseTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		//give is some movement
		modelMatrix.identity();
		modelMatrix.Translate(-2.5f, 1.25f, 0.0f);
		modelMatrix.Scale(0.5f, 0.5f, 1.0f);
		//all the transformation to be added
		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);
		//draw it
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//upper right base
		glBindTexture(GL_TEXTURE_2D, ulBase);
		//set the vertices of image
		float urBaseV[] = { -1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, urBaseV);
		glEnableVertexAttribArray(program.positionAttribute);
		//show all of image
		float urBaseTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, urBaseTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		//give is some movement
		modelMatrix.identity();
		modelMatrix.Translate(2.5f, 1.25f, 0.0f);
		modelMatrix.Rotate(-90.0*(3.141592653 / 180));
		modelMatrix.Scale(0.5f, 0.5f, 1.0f);
		//all the transformation to be added
		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);
		//draw it
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//lower right base
		glBindTexture(GL_TEXTURE_2D, ulBase);
		//set the vertices of image
		float lrBaseV[] = { -1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, lrBaseV);
		glEnableVertexAttribArray(program.positionAttribute);
		//show all of image
		float lrBaseTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, lrBaseTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		//give is some movement
		modelMatrix.identity();
		modelMatrix.Translate(2.5f, -1.25f, 0.0f);
		modelMatrix.Rotate(-180.0*(3.141592653 / 180));
		modelMatrix.Scale(0.5f, 0.5f, 1.0f);
		//all the transformation to be added
		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);
		//draw it
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//lower left base
		glBindTexture(GL_TEXTURE_2D, ulBase);
		//set the vertices of image
		float llBaseV[] = { -1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, llBaseV);
		glEnableVertexAttribArray(program.positionAttribute);
		//show all of image
		float llBaseTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, llBaseTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		//give is some movement
		modelMatrix.identity();
		modelMatrix.Translate(-2.5f, -1.25f, 0.0f);
		modelMatrix.Rotate(90.0*(3.141592653 / 180));
		modelMatrix.Scale(0.5f, 0.5f, 1.0f);
		//all the transformation to be added
		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);
		//draw it
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//the moving ball from base to base
		glBindTexture(GL_TEXTURE_2D, movingBall);
		float movingBallV[] = { -1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, movingBallV);
		glEnableVertexAttribArray(program.positionAttribute);
		float movingBallTextCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, movingBallTextCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		modelMatrix.identity();
		modelMatrix.Scale(0.5f, 0.5f, 1.0f);
		//moving between all bases
		if (up) {
			movingBally += speed * elapsed;
			if (movingBally >= 2.5f) {
				up = false;
				right = true;
			}
		}
		else if (right) {
			movingBallx += speed * elapsed;
			if (movingBallx >= 4.5f) {
				right = false;
				down = true;
			}
		}
		else if (down) {
			movingBally -= speed * elapsed;
			if (movingBally <= -2.5f) {
				down = false;
				left = true;
			}
		}
		else if (left) {
			movingBallx -= speed * elapsed;
			if (movingBallx <= -4.5f) {
				left = false;
				up = true;
			}
		}
		modelMatrix.Translate(movingBallx, movingBally, 0);
		modelMatrix.Rotate(angle*(3.141592653/180));
		//the updates onto the program
		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);
		//draw it
		glDrawArrays(GL_TRIANGLES, 0, 6);

		
		//center spinning ball thing
		glBindTexture(GL_TEXTURE_2D, middleBall);
		float ballVertices[] = { -1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, ballVertices);
		glEnableVertexAttribArray(program.positionAttribute);
		//show all of image
		float texCoords2[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords2);
		glEnableVertexAttribArray(program.texCoordAttribute);
		modelMatrix.identity();
		modelMatrix.Scale(0.5f, 0.5f, 1.0f);
		modelMatrix.Rotate(-1 * angle * 2.0f *  (3.141592653 / 180));
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
