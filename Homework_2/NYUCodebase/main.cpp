//Luvneesh Mugrai
//Intro to Game Programming
//Homework_2
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
#include <vector>
#include <iostream>
#include <fstream>

using namespace std;

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

class Entity{
public:
	float centerX, centerY, width, height;
	Matrix modelMatrix;
	float speedX = 0.0f;
	float speedY = 0.0f;
	bool movingY; 
	bool movingX;
	std::string type;
	//float vertices[] = { -0.125, -0.5, 0.125, -0.5, 0.125, 0.5, -0.125, -0.5, 0.125, 0.5, -0.125, 0.5 };

	Entity(float cX, float cY, float w, float h, bool mY, bool mX, std::string t) : 
		centerX(cX), centerY(cY), width(w), height(h), movingY(mY), movingX(mX), type(t){
	}

	void update(float elapsed/*, std::vector<Entity>& allEntities*/){
		//for (int i = 0; i < allEntities.size(); i++){
		//	if (&allEntities[i] != this)
		//	{
		//		if (this->type == "ball")
		//			collisionCheck(allEntities[i]);
		//	}
		//}

		modelMatrix.identity();
		if (movingY){
			centerY += speedY * elapsed;
		}
		if (movingX){
			centerX += speedX * elapsed;
		}
		modelMatrix.Translate(centerX, centerY, 0.0f);
		/*if (type == "wall"){
			modelMatrix.Translate(centerX, 1.0f, 0.0f);
		}*/
		if (type == "paddle")
			modelMatrix.Scale(width/2, height/2, 0.0f);//(0.125f, 0.5f, 0.0f);
		else if (type == "ball")
			modelMatrix.Scale(width/2, height/2, 0.0f);//(0.1f, 0.1f, 0.0f);
		else if (type == "wall")
			modelMatrix.Scale(width/2, height/2, 0.0f);
	}

	void draw(ShaderProgram program, GLuint img){
		glBindTexture(GL_TEXTURE_2D, img);
		float vertices[] = /*{ -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };*/{ -1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1 };//{ -0.125, -0.5, 0.125, -0.5, 0.125, 0.5, -0.125, -0.5, 0.125, 0.5, -0.125, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);
		//show all of image
		float textCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, textCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		program.setModelMatrix(modelMatrix);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
	}

	void collisionCheck(Entity r1){ //current is R2

		//collide
		if (!((r1.centerY - r1.height / 2 >= this->centerY + this->height / 2) || //r1 bot > r2 top
			(r1.centerY + r1.height / 2 <= this->centerY - this->height / 2) || //r1 top < r2 bot
			(r1.centerX - r1.width / 2 >= this->centerX + this->width / 2) || //r1 left > r2 right
			(r1.centerX + r1.width / 2 <= this->centerX - this->width / 2) //r1 right < r2 left
			)){

			//DELETE THIS


			if (r1.type == "paddle"){
				ofstream log;
				log.open("log.txt");
				log << r1.type << r1.centerX << " " << r1.centerY << " " << r1.width / 2 << " " << r1.height / 2 << endl;
				log << this->type << centerX << " " << centerY << " " << width / 2 << " " << height / 2 << endl;
				log << endl << r1.centerY - r1.height / 2 << " > " << this->centerY + this->height / 2;
				log << endl << r1.centerY + r1.height / 2 << " < " << this->centerY - this->height / 2;
				log << endl << r1.centerX - r1.width / 2 << " > " << this->centerX + this->width / 2;
				log << endl << r1.centerX + r1.width / 2 << " < " << this->centerX - this->width / 2;
				log << endl << r1.centerY - r1.height / 2 << " <= " << this->centerY - this->height / 2;
				log.close();
			}
			//hit a wall
			if (r1.type == "wall"){
				//I am a ball
				if (this->type == "ball"){
					this->speedY *= -1;
				}
				//I am a paddle....
				if (this->type == "paddle"){
					if (this->centerY > 0){
						this->centerY = 2.0f - r1.height - this->height / 2;
					}
					else if (this->centerY <= 0){
						this->centerY = -1 * 2.0f + r1.height + this->height / 2;
					}
				}
			}
			//hit a paddle
			else if (r1.type == "paddle"){
				//be specific please
				//left/right
				if ( ((r1.centerX + r1.width / 2 >= this->centerX - this->width / 2) || (r1.centerX - r1.width / 2 <= this->centerX + this->width / 2)) &&
					(r1.centerY + r1.height/2 >= this->centerY + this->height/2) && (r1.centerY - r1.height/2 <= this->centerY - this->height/2) ){
					this->speedX *= -1;
				}
				//hit bot/top
				else if ((r1.centerY - r1.height / 2 <= this->centerY + this->height / 2) || (r1.centerY + r1.height / 2 >= this->centerY - this->height / 2)){
					this->speedY *= -1;
				}
			}
		}
	}
	//if (!(leftPad.centerY - leftPad.height / 2 >= ball.yCoord + ball.height / 2) && //r1 bot > r2 top
	//	!(leftPad.centerY + leftPad.height / 2 <= ball.yCoord - ball.height / 2) && //r1 top < r2 bot
	//	!(leftPad.centerX - leftPad.width / 2 >= ball.xCoord + ball.width / 2) && //r1 left > r2 right
	//	!(leftPad.centerX + leftPad.width / 2 <= ball.xCoord - ball.width / 2) //r1 right < r2 left
	//	){
	//	ball.xDir *= -1;
	//}

};
/*
class Paddle{
public:
	float top, bot, left, right, centerX, centerY,  width,  height; 

	float paddleSpeed = 0.0f;

	Paddle(float t, float b, float l, float r, float cx, float cy, float w, float y) : top(t), bot(b), left(l), right(r), centerX(cx), centerY(cy), width(w), height(y){ }
};

class Ball{
public:
	float width;
	float height;

	float xCoord = 0.0f;
	float yCoord = 0.0f;
	float xDir = -1.0f;//(float)rand();
	float yDir = -1.0f;// (float)rand(); //FIX THIS
	float vel = 1.0f;
	float accel = 0.05f;

	Ball(float xC, float yC,float w, float h) : xCoord(xC), yCoord(yC), width(w), height(h) {}

	void movement(float elapsed){
		xCoord += xDir * vel * elapsed;
		std::cout << yCoord << std::endl;
		yCoord += yDir * vel * elapsed;
	}
};
*/
int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

		//left Paddle
		Entity leftPaddleE(-3.125f, 0.0f, 0.5f, 1.0f, true, false, "paddle");
		GLuint leftPaddle = LoadTexture(RESOURCE_FOLDER"PaddleSprite.png");
		//right Paddl
		Entity rightPaddleE(3.125f, 0.0f, 0.5f, 1.0f, true, false, "paddle");
		GLuint rightPaddle = LoadTexture(RESOURCE_FOLDER"PaddleSprite.png");
		//ball
		Entity ballE(0.0f, 0.0f, 0.2f, 0.2f, true, true, "ball");
		ballE.speedX = -0.50f;//rand() % (3 - 1 + 1) + 1;
		ballE.speedY = -0.50f;//rand() % (3 - 1 + 1) + 1;
		GLuint ballSprite = LoadTexture(RESOURCE_FOLDER"BallSprite.png");
		//walls
		GLuint wallSprite = LoadTexture(RESOURCE_FOLDER"WallSprite.png");
		Entity topWallE(0.0f, 1.95f, 3.55 * 2, 0.1, false, false, "wall");
		Entity botWallE(0.0f, -1.95f, 3.55 * 2, 0.1, false, false, "wall");
		
		//std::vector<Entity> allEntity = { leftPaddleE, rightPaddleE, ballE, topWallE, botWallE };
		
		bool gameOn = false;

		float lastTick = 0;

		glViewport(0, 0, 640, 360);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
		Matrix projectionMatrix;
		Matrix modelMatrix;
		Matrix viewMatrix;
		projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
		glUseProgram(program.programID);


	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			} 
			else if (event.type == SDL_KEYDOWN){
				//left paddle up
				if (event.key.keysym.scancode == SDL_SCANCODE_W){
					leftPaddleE.speedY = 1.0f;
				}
				//left paddle down
				else if (event.key.keysym.scancode == SDL_SCANCODE_S){
					leftPaddleE.speedY = -1.0f;
				}
				//right paddle up
				if (event.key.keysym.scancode == SDL_SCANCODE_UP){
					rightPaddleE.speedY = 1.0f;
					//rightPad.paddleSpeed = 1.0f;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_DOWN){
					rightPaddleE.speedY = -1.0f;
					//rightPad.paddleSpeed = -1.0f;
				}
				//start game
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE){
					if (!gameOn){
						gameOn = true;
					}
				}
			}
			//player let go 
			else if (event.type == SDL_KEYUP){
				//left player
				if (event.key.keysym.scancode == SDL_SCANCODE_W || event.key.keysym.scancode == SDL_SCANCODE_S){
					leftPaddleE.speedY = 0.0f;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_UP || event.key.keysym.scancode == SDL_SCANCODE_DOWN){
					rightPaddleE.speedY = 0.0f;
				}
			}
		}

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastTick; 
		lastTick = ticks;

		glClear(GL_COLOR_BUFFER_BIT);

		//walls
		//top
		topWallE.update(elapsed/*, allEntity*/);
		topWallE.draw(program, wallSprite);
		//bot
		botWallE.update(elapsed/*, allEntity*/);
		botWallE.draw(program, wallSprite);

		//left paddle entity
		leftPaddleE.update(elapsed/*, allEntity*/);
		//draw
		leftPaddleE.draw(program, leftPaddle);
		//glBindTexture(GL_TEXTURE_2D, leftPaddle);
		//float leftPaddleVert[] = { -0.125, -0.5, 0.125, -0.5, 0.125, 0.5, -0.125, -0.5, 0.125, 0.5, -0.125, 0.5 };
		//glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, leftPaddleVert);
		//glEnableVertexAttribArray(program.positionAttribute);
		////show all of image
		//float leftPaddleTextCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		//glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, leftPaddleTextCoords);
		//glEnableVertexAttribArray(program.texCoordAttribute);
		//program.setModelMatrix(leftPaddleE.modelMatrix);
		//glDrawArrays(GL_TRIANGLES, 0, 6);
		//glDisableVertexAttribArray(program.positionAttribute);
		//glDisableVertexAttribArray(program.texCoordAttribute);


		/*
		//left paddle
		glBindTexture(GL_TEXTURE_2D, leftPaddle);
		//set vertices
		float leftPaddleVert[] = { -0.125, -0.5, 0.125, -0.5, 0.125, 0.5, -0.125, -0.5, 0.125, 0.5, -0.125, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, leftPaddleVert);
		glEnableVertexAttribArray(program.positionAttribute);
		//show all of image
		float leftPaddleTextCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, leftPaddleTextCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		modelLeftPadd.identity();
		//keep track of center
		leftPad.centerY += leftPad.paddleSpeed * elapsed;
		modelLeftPadd.Translate(leftPad.centerX, leftPad.centerY, 0.0f);

		program.setModelMatrix(modelLeftPadd);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		*/
		//right paddle
		rightPaddleE.update(elapsed/*, allEntity*/);
		rightPaddleE.draw(program, rightPaddle);
		/*glBindTexture(GL_TEXTURE_2D, rightPaddle);
		//vertices
		float rightPaddleVert[] = { -0.125, -0.5, 0.125, -0.5, 0.125, 0.5, -0.125, -0.5, 0.125, 0.5, -0.125, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, rightPaddleVert);
		glEnableVertexAttribArray(program.positionAttribute);
		//image
		float rightPaddleTextCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, rightPaddleTextCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		modelRightPadd.identity();
		//track center
		rightPad.centerY += rightPad.paddleSpeed * elapsed;
		modelRightPadd.Translate(rightPad.centerX, rightPad.centerY, 0.0f);

		program.setModelMatrix(modelRightPadd);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		*/
		//udpate p1
		//ball collision
		ballE.collisionCheck(topWallE);
		ballE.collisionCheck(botWallE);
		ballE.collisionCheck(leftPaddleE);
		ballE.collisionCheck(rightPaddleE);
		//update p2
		ballE.update(elapsed/*, allEntity*/);
		ballE.draw(program, ballSprite); 
		//player collision
		leftPaddleE.collisionCheck(topWallE);
		leftPaddleE.collisionCheck(botWallE);
		rightPaddleE.collisionCheck(topWallE);
		rightPaddleE.collisionCheck(botWallE);

		

		//glBindTexture(GL_TEXTURE_2D, ballSprite);
		////vertices
		//float ballV[] = { -0.1, -0.1, 0.1, -0.1, 0.1, 0.1, -0.1, -0.1, 0.1, 0.1, -0.1, 0.1 };
		//glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, ballV);
		//glEnableVertexAttribArray(program.positionAttribute);
		////uv
		//float ballTextCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		//glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, ballTextCoords);
		//glEnableVertexAttribArray(program.texCoordAttribute);
		//
		//modelBall.identity();
		//
		////collision checks
		////left paddle
		///*if (!(leftPad.centerY - leftPad.height / 2 >= ball.yCoord + ball.height / 2) && //r1 bot > r2 top
		//	!(leftPad.centerY + leftPad.height / 2 <= ball.yCoord - ball.height / 2) && //r1 top < r2 bot
		//	!(leftPad.centerX - leftPad.width / 2 >= ball.xCoord + ball.width / 2) && //r1 left > r2 right
		//	!(leftPad.centerX + leftPad.width / 2 <= ball.xCoord - ball.width / 2) //r1 right < r2 left
		//	){
		//	ball.xDir *= -1;
		//}*/

		////collision with screen boundaries
		////lower wall || upper wall
		//if ((ball.yCoord + ball.height / 2) >= 2.0 || (ball.yCoord - ball.height / 2) <= -2.0f){
		//	ball.yDir *= -1;
		//	//ball.vel += ball.accel; //make it a bit faster
		//} 

		//if (ball.xCoord <= -3.55f || ball.xCoord >= 3.55f){
		//	ball.xDir *= -1;
		//	ball.yCoord = 0;
		//	ball.xCoord = 0;
		//}

		////no collision
		////just keep moving
		//ball.movement(elapsed);
		//modelBall.Translate(ball.xCoord, ball.yCoord, 0.0f);
		//program.setModelMatrix(modelBall);
		//glDrawArrays(GL_TRIANGLES, 0, 6);

		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);
		
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
