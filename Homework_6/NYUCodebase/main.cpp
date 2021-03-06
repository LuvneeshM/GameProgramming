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

#include <Windows.h>

#include <SDL_mixer.h>

using namespace std;

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

//gloabl stuff
int winner = 0;
bool gameOn = false;

//int Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize);
//
//Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

Mix_Chunk *hitWall;
Mix_Chunk *hitPaddle;
Mix_Music *music;


void DrawText(ShaderProgram *program, int fontTexture, std::string text, float size, float spacing) {
	float texture_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;
	for (int i = 0; i < text.size(); i++) {
		float texture_x = (float)(((int)text[i]) % 16) / 16.0f;
		float texture_y = (float)(((int)text[i]) / 16) / 16.0f;
		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
		});
	}
	glUseProgram(program->programID);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

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
	float angleTweak = 4.0f;
	float speedX = 0.0f;
	float speedY = 0.0f;
	bool movingY; 
	bool movingX;
	string type;

	Entity* hit = NULL;
	//float vertices[] = { -0.125, -0.5, 0.125, -0.5, 0.125, 0.5, -0.125, -0.5, 0.125, 0.5, -0.125, 0.5 };

	Entity(){}

	Entity(float cX, float cY, float w, float h, bool mY, bool mX, std::string t) : 
		centerX(cX), centerY(cY), width(w), height(h), movingY(mY), movingX(mX), type(t){
	}

	void update(float elapsed){
		modelMatrix.identity();
		if (movingY){
			if (type == "ball") {
				centerY += speedY*cos(3.14159265 / angleTweak)*elapsed*1.25f;
			}
			centerY += speedY *elapsed;
			
		}
		if (movingX){
			if (type == "ball") {
				centerX += speedX * sin(3.14159265 / angleTweak)*elapsed*1.5f;
			}
			centerX += speedX * elapsed;
		}
		modelMatrix.Translate(centerX, centerY, 0.0f);

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

	void collisionCheck(Entity& r1){ 
		//did I hit you before (only ball scenario)
		if (hit == NULL || hit != &r1 || this->type != "ball") {
			
			//collide
			if (!(r1.centerY - r1.height / 2 >= this->centerY + this->height / 2) && //r1 bot > r2 top
				!(r1.centerY + r1.height / 2 <= this->centerY - this->height / 2) && //r1 top < r2 bot
				!(r1.centerX - r1.width / 2 >= this->centerX + this->width / 2) && //r1 left > r2 right
				!(r1.centerX + r1.width / 2 <= this->centerX - this->width / 2) //r1 right < r2 left
				) {
				hit = &r1;

				//hit a wall
				if (r1.type == "wall") {
					//I am a ball
					if (this->type == "ball") {
						this->speedY *= -1;
						Mix_PlayChannel(-1, hitWall, 0);
					}
					//I am a paddle....
					if (this->type == "paddle") {
						if (this->centerY > 0) {
							this->centerY = 2.0f - r1.height - this->height / 2;
						}
						else if (this->centerY <= 0) {
							this->centerY = -1 * 2.0f + r1.height + this->height / 2;
						}
					}
				}
				//hit a paddle
				else if (r1.type == "paddle") {
					Mix_PlayChannel(2, hitPaddle, 0);
					//be specific
					//left/right
					//left
					if (speedX < 0 && r1.centerX + r1.width / 2 >= this->centerX - this->width / 2 && r1.centerX+r1.width/2 <= centerX) {
						if (r1.centerY + r1.height - 0.005f >= this->centerY + this->height) {
							this->speedX *= -1;
						}
						else if (r1.centerY - r1.height + 0.005f <= this->centerY - this->height) {
							this->speedX *= -1;
						}
					}
					//right
					else if (r1.centerX - r1.width / 2 <= this->centerX + this->width / 2 && r1.centerX-r1.width/2 >= this->centerX) {
						if (r1.centerY + r1.height - 0.005f >= this->centerY + this->height) {
							this->speedX *= -1;
						}
						else if (r1.centerY - r1.height + 0.005f <= this->centerY - this->height) {
							this->speedX *= -1;
						}
					}

					//hit bot/top
					if ( ((r1.centerY - r1.height / 2 <= this->centerY + this->height / 2) && (r1.centerY - r1.height /2 >= this->centerY - this->height/2)) || 
						((r1.centerY + r1.height / 2 >= this->centerY - this->height / 2) && (r1.centerY + r1.height/2 <= this->centerY + this->height/2)) ) {
						this->speedY *= -1;
					}
				}

				//change up the angle for fun
 				int change = rand() % 5;
				if (change == 0) {
					angleTweak = 4.0f;
				}
				else if (change == 1) {
					angleTweak = 3.5f;
				}
				else if (change == 2) {
					angleTweak = 3.75f;
				}
				else if (change == 3) {
					angleTweak = 4.25f;
				}
				else {
					angleTweak = 4.5f;
				}


			}
			//did I win
			else if (this->type == "ball") {
				//right wins 
				if (this->centerX - this->width / 2 <= -3.55f) {
					winner = 1;
					gameOn = false;
				}
				//left wins
				else if (this->centerX + this->width / 2 >= 3.55f) {
					winner = 2;
					gameOn = false;
				}
			}
		}
	}//end collision

	void entityToString() {
		ostringstream ss;
		ss << this->type << " stats: centerX " << centerX << " centerY " << centerY << " width " << width << " height " << height
			<< " speedX " << speedX << " speedY" << speedY;
		OutputDebugString( ss.str().c_str());
	}

};

void setUp(Entity& leftPaddleE, Entity& rightPaddleE, Entity& ballE){
	leftPaddleE = Entity(-3.125f, 0.0f, 0.5f, 1.0f, true, false, "paddle");
	rightPaddleE = Entity(3.125f, 0.0f, 0.5f, 1.0f, true, false, "paddle");
	ballE = Entity(0.0f, 0.0f, 0.2f, 0.2f, true, true, "ball");
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

		Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

		hitWall = Mix_LoadWAV("sub.wav");
		hitPaddle = Mix_LoadWAV("Sonar2.wav");
		music = Mix_LoadMUS("music.mp3");

		Mix_PlayMusic(music, -1);

		//left Paddle
		Entity leftPaddleE;
		GLuint leftPaddle = LoadTexture(RESOURCE_FOLDER"PaddleSprite.png");
		//right Paddl
		Entity rightPaddleE;
		GLuint rightPaddle = LoadTexture(RESOURCE_FOLDER"PaddleSprite.png");
		//ball
		Entity ballE;
		GLuint ballSprite = LoadTexture(RESOURCE_FOLDER"BallSprite.png");	
		
		//walls
		GLuint wallSprite = LoadTexture(RESOURCE_FOLDER"WallSprite.png");
		Entity topWallE(0.0f, 1.95f, 3.55 * 2, 0.1, false, false, "wall");
		Entity botWallE(0.0f, -1.95f, 3.55 * 2, 0.1, false, false, "wall");
				
		//font
		GLuint font = LoadTexture(RESOURCE_FOLDER"font1.png");
		string whoWon = "";

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
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE){
					if (!gameOn){
						gameOn = true;
						setUp(leftPaddleE, rightPaddleE, ballE);
						ballE.speedX = ((rand() % 2 + 1) == 1) ? 1.75f : -1.55f;
						ballE.speedY = ((rand() % 2 + 1) == 1) ? 1.65f : -1.85f;
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

		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		//left paddle
		if (keys[SDL_SCANCODE_W]) {
			if (abs(ballE.speedX) < 1.70f || abs(ballE.speedY) < 1.70f)
				leftPaddleE.speedY = 1.95f;
			else
				leftPaddleE.speedY = 2.5f;
		}
		else if (keys[SDL_SCANCODE_S]) {
			if (abs(ballE.speedX) < 1.70f || abs(ballE.speedY) < 1.70f)
				leftPaddleE.speedY = -1.95f;
			else
				leftPaddleE.speedY = -2.5f;
		}
		//right paddle
		if (keys[SDL_SCANCODE_UP]) {
			if (abs(ballE.speedX) < 1.70f || abs(ballE.speedY) < 1.70f)
				rightPaddleE.speedY = 1.95f;
			else
				rightPaddleE.speedY = 2.5f;
		}
		else if (keys[SDL_SCANCODE_DOWN]) {
			if (abs(ballE.speedX) < 1.70f || abs(ballE.speedY) < 1.70f)
				rightPaddleE.speedY = -1.95f;
			else
				rightPaddleE.speedY = -2.5f;
		}

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastTick; 
		lastTick = ticks;

		glClear(GL_COLOR_BUFFER_BIT);


		//walls
		//top
		topWallE.update(elapsed);
		topWallE.draw(program, wallSprite);
		//bot
		botWallE.update(elapsed);
		botWallE.draw(program, wallSprite);

		if (gameOn){
			//udpate p1
			//ball collision
			ballE.collisionCheck(topWallE);
			ballE.collisionCheck(botWallE);
			ballE.collisionCheck(leftPaddleE);
			ballE.collisionCheck(rightPaddleE);
			//update p2
			//player collision
			leftPaddleE.collisionCheck(topWallE);
			leftPaddleE.collisionCheck(botWallE);
			rightPaddleE.collisionCheck(topWallE);
			rightPaddleE.collisionCheck(botWallE);
			//updates
			//paddles
			leftPaddleE.update(elapsed);
			rightPaddleE.update(elapsed);
			//ball
			ballE.update(elapsed);
		}
		//
		//draw
		leftPaddleE.draw(program, leftPaddle);
		rightPaddleE.draw(program, rightPaddle);
		ballE.draw(program, ballSprite);

		//winner?
		if (!gameOn){
			modelMatrix.identity();
			modelMatrix.Translate(-2.5f, 1.5f, 0.0f);
			program.setModelMatrix(modelMatrix);
			string help = "Press Space to Start";
			DrawText(&program, font, help, 0.25f, 0);
			modelMatrix.identity();
			modelMatrix.Translate(-2.5f, -1.0f, 0.0f);
			program.setModelMatrix(modelMatrix);
			if (winner == 1){
				whoWon = "Right Player Won";
				DrawText(&program, font, whoWon, 0.25f, 0);
			}
			else if (winner == 2){
				whoWon = "Left Player Won";
				DrawText(&program, font, whoWon, 0.25f, 0);
			}

		}

		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);
		
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		
		SDL_GL_SwapWindow(displayWindow);
	}

	//cleanup
	Mix_FreeChunk(hitWall);
	Mix_FreeChunk(hitPaddle);
	Mix_FreeMusic(music);

	SDL_Quit();
	return 0;
}
