//Luvneesh Mugrai
//Intro to Game Programming
//Space Inavders

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
#include <tuple>

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

class SheetSprite {
public:
	SheetSprite(unsigned int xtextureID, float tu, float tv, float twidth, float theight, float tsize) {
		u = tu;
		v = tv;
		width = twidth;
		height = theight;
		size = tsize;
		textureID = xtextureID;
	}
	SheetSprite() {}
	void Draw(ShaderProgram *program);

	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};
void SheetSprite::Draw(ShaderProgram *program) {

	GLfloat texCoords[] = {
		u, v + height,
		u + width, v,
		u, v,
		u + width, v,
		u, v + height,
		u + width, v + height
	};

	float aspect = width / height;

	float vertices[] = {
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, 0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, -0.5f * size
	};
	glUseProgram(program->programID);

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	
	glBindTexture(GL_TEXTURE_2D, textureID);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);

}

class Entity;

bool win = false;

//for player
GLuint ludioSheet;
//player animations
int ludioIndex = 0;
float ludioSize = 1.0f;
vector<tuple<float, float, float, float>> ludioMovesData;

//for enemies
GLuint enemiesSheet;
vector<tuple<float, float, float, float>> enemiesAnimData;
int flyIndex = 0;
int glopIndex = 2;
int snailIndex = 4;
float enSize = 0.5f;
//platform
vector<Entity> platform;



float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t*v1;
}

class Entity {
	bool left = false;
	bool right = true;
public: 
	Entity() {}

	Entity(float x, float y, float width, float height, string type, float xVel, float yVel, bool alive) {
		this->x = x;
		this->y = y;
		this->width = width;
		this->height = height;
		this->type = type;
		this->yVel = yVel;
		this->xVel = xVel;
		this->alive = alive;
		//for animation
		ludioNumFrames = 11;
		animationElapsed = 0.0f;
		framesPerSecond = 15.0f;
	}
	void update(float elapsed) {
		if (alive) {
			//used for all animations
			//each entity has its own animation time
			animationElapsed += elapsed;
			if (type == "player") {
				yVel = lerp(yVel, 0.0f, elapsed*friction_y);
				//only jump once
				if (!jump) {
					const Uint8 *keys = SDL_GetKeyboardState(NULL);
					if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_SPACE]) {
						if (collidedBottom == true) yVel = 4.0f;
						jump = true;
					}
					//animate walking man
					if (animationElapsed > 1.0f / framesPerSecond) {
						sprite = SheetSprite(ludioSheet, get<0>(ludioMovesData[ludioIndex]) / 512.0f,
							get<1>(ludioMovesData[ludioIndex]) / 512.0f, get<2>(ludioMovesData[ludioIndex]) / 512.0f,
							get<3>(ludioMovesData[ludioIndex]) / 512.0f, ludioSize);
						ludioIndex = (ludioIndex + 1) % ludioMovesData.size();
						animationElapsed = 0.0f;
					}
				}
				//change sprite to jump one
				else if (jump) {
					sprite = SheetSprite(ludioSheet, 438.0f / 512.0f,
						93.0f / 512.0f, 67.0f / 512.0f,
						94.0f / 512.0f, ludioSize);
				}
				yVel += acceleration_y * elapsed;
				y += yVel * elapsed;
				collideWithGroundY();
			}
			//fly animated
			else {

				//the enemies
				if (type == "fly") {
					if (animationElapsed > 1.0f / framesPerSecond) {
						sprite = SheetSprite(enemiesSheet, get<0>(enemiesAnimData[flyIndex]) / 353.0f,
							get<1>(enemiesAnimData[flyIndex]) / 153.0f, get<2>(enemiesAnimData[flyIndex]) / 353.0f,
							get<3>(enemiesAnimData[flyIndex]) / 153.0f, enSize);
						flyIndex = (flyIndex + 1) % 2;
						animationElapsed = 0.0f;
					}
					//check for collision with player
					//check for "collision" off screen
				}
				//glop anim
				if (type == "glop") {
					if (animationElapsed > 1.0f / framesPerSecond) {
						sprite = SheetSprite(enemiesSheet, get<0>(enemiesAnimData[glopIndex]) / 353.0f,
							get<1>(enemiesAnimData[glopIndex]) / 153.0f, get<2>(enemiesAnimData[glopIndex]) / 353.0f,
							get<3>(enemiesAnimData[glopIndex]) / 153.0f, enSize);
						glopIndex = ((glopIndex + 1) % 2) + 2;
						animationElapsed = 0.0f;
					}
					//fall down to ground
					yVel += acceleration_y * elapsed;
					y += yVel *elapsed;
					collideWithGroundY();
					//check for collision with player
					//check for "collision" off screen
				}
				if (type == "snail") {
					if (animationElapsed > 1.0f / framesPerSecond) {
						sprite = SheetSprite(enemiesSheet, get<0>(enemiesAnimData[snailIndex]) / 353.0f,
							get<1>(enemiesAnimData[snailIndex]) / 153.0f, get<2>(enemiesAnimData[snailIndex]) / 353.0f,
							get<3>(enemiesAnimData[snailIndex]) / 153.0f, enSize);
						snailIndex = ((snailIndex + 1) % 2) + 4;
						animationElapsed = 0.0f;
					}
					yVel += acceleration_y * elapsed;
					y += yVel *elapsed;
					collideWithGroundY();
					//check for collision with player
					//check for "collision" off screen
				}
				if (xVel <= 5.0f) {
					xVel += acceleration_x * elapsed;
				}
				x -= xVel * elapsed;
			}
		}
	}

	void collideWithGroundY() {
		//check with only 1st row 
		//1st row all same so check any of the 15 y
			if (this->y-height/2 < platform[0].y+platform[0].height/2-0.5) {
				collidedBottom = true;
				yVel = 0.0f;
				acceleration_y = 0.0f;
				pen_y = (platform[0].height/2 + platform[0].y-0.5) - (y-height/2);
				y += pen_y + 0.002;
				jump = false;
			}
			else {
				collidedBottom = false;
				acceleration_y = -3.8;
				pen_y = 0;
			
			}
		
	}


	void draw(ShaderProgram *program) {
		if (alive) {
			modelMatrix.identity();
			modelMatrix.Translate(x, y, 0);
			program->setModelMatrix(modelMatrix);
			sprite.Draw(program);
		}
	}

	string type;
	float xVel;
	float yVel = 0.0f;

	float x;
	float y;

	float acceleration_y = 0.0f;
	float acceleration_x = 0.0f;
	
	float friction_y = 0.5f;
	float friction_x = 0.5f;

	float width;
	float height;

	bool alive;
	bool jump = false;

	float pen_y = 0;
	float pen_x = 0;

	//collisions
	bool collidedTop = false;
	bool collidedBottom = false;
	bool collidedLeft = false;
	bool collidedRight = false;

	Matrix modelMatrix;
	SheetSprite sprite;
	//animtion
	int ludioNumFrames;
	float animationElapsed;
	float framesPerSecond;
	

	void entityToString() {
		ostringstream ss;
		ss << this->type << " stats: centerX " << x << " centerY " << y << " width " << width << " height " << height
			<< " speedX " << xVel << " speedY" << yVel;
		OutputDebugString(ss.str().c_str());
	}
}; //end entity

 
//set up for the game
//used at start and when player restarts
void setUp(vector<Entity>& platform, const GLuint platSheet, Entity& player, const GLuint playerSheet, vector<Entity>& e, const GLuint eSheets) {
	//platform
	float platSize = .750f;
	float platW = 70.0f;
	float platH = 70.0f;
	float platAspect = platW / platH;
	//top layer
	float platformX = 3.5f;
	float platformY = -1.0f;
	for (int i = 0; i < 15; i++) {
		Entity platformT  = Entity(platformX, platformY, 0.5*platSize*platAspect * 2, 0.5f*platSize * 2, "platformTop", 0.0f, 0.0f, true);
		platformT.sprite = SheetSprite(platSheet, 144.0f / 1024.0f, 792.0f / 1024.0f, platW / 1024.0f, platH / 1024.0f, platSize);
		platform.push_back(platformT);
		//right to left fill 
		//last is first 
		platformX -= 0.74001f;
	}
	//bot layer
	platformX = 3.5f;
	platformY = platformY - platSize +0.01;
	for (int i = 0; i < 15; i++) {
		Entity platformT = Entity(platformX, platformY, 0.5*platSize*platAspect * 2, 0.5f*platSize * 2, "platformBot", 0.0f, 0.0f, true);
		platformT.sprite = SheetSprite(platSheet, 720.0f / 1024.0f, 864.0f / 1024.0f, platW / 1024.0f, platH / 1024.0f, platSize);
		platform.push_back(platformT);
		//right to left fill 
		//last is first 
		platformX -= 0.74001f;
	}
	
	//player
	float playerSize = 1.0;
	float playerW = 66.0f;
	float playerH = 92.0f;
	float playerAspect = playerW / playerH;
	float playerX = -2.0f;
	float playerY = 0.0f;
	player = Entity(playerX, playerY, 0.5*playerSize*playerAspect * 2.0f, 0.5f*playerSize*2.0f, "player", 0.0f, 0.0f, true);
	player.sprite = SheetSprite(playerSheet, 67.0f / 512.0f, 196.0f / 512.0f, playerW / 512.0f, playerH / 512.0f, playerSize);
	//enemies
	//starting 
	//1 of each
	//fly walk 1
	float flySize = 0.5f;
	float flyW = 72.0f;
	float flyH = 36.0f; 
	float flyAsp = flyW / flyH;
	float flyX = 0.0f;
	float flyY = 0.0f;
	Entity fly = Entity(flyX, flyY, 0.5*flySize*flyAsp * 2, 0.5f*flySize * 2, "fly", 1.0f, 0.0f, true);
	fly.sprite = SheetSprite(eSheets, 0.0f / 353.0f, 32.0f / 153.0f, flyW / 353.0f, flyH / 153.0f, flySize); //mult flysize by fly aspect and the spritesheet aspect
		/*SheetSprite(enemiesSheet, get<0>(enemiesAnimData[0]) / 353.0f,
			get<1>(enemiesAnimData[0]) / 153.0f, get<2>(enemiesAnimData[0]) / 353.0f,
 			get<3>(enemiesAnimData[0]) / 153.0f, enSize);*/
	e.push_back(fly);
	//glop walk 
	float glopSize = 0.5f;
	float glopW = 50.0f;
	float glopH = 28.0f;
	float glopAsp = glopW / glopH;
	float glopX = 1.2f;
	float glopY = 0.0f;
	Entity glop = Entity(glopX, glopY, 0.5*glopSize*glopAsp * 2, 0.5f*glopSize * 2, "glop", 1.4f, 0.0f, true);
	glop.sprite = SheetSprite(eSheets, 52.0f / 353.0f, 125.0f / 153.0f, glopW / 353.0f, glopH / 153.f, glopSize);
	e.push_back(glop);
	//snail walk 1
	float snailSize = 0.5f;
	float snailW = 54.0f;
	float snailH = 31.0f;
	float snailAsp = snailW / snailH;
	float snailX = -0.75f;
	float snailY = 0.0f;
	Entity snail = Entity(snailX, snailY, 0.5*snailSize*snailAsp * 2, 0.5f*snailSize * 2, "snail", 0.75f, 0.0f, true);
	snail.sprite = SheetSprite(eSheets, 143.0f / 353.0f, 34.0f / 153.0f, snailW / 353.0f, snailH / 153.f, snailSize);
	e.push_back(snail);
}


int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Invaders of Space", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif
		
		//font
		GLuint font = LoadTexture("font1.png");
				
		//for player animations
		ludioMovesData.push_back(tuple<float, float, float, float>(0, 0, 72, 97));
		ludioMovesData.push_back(tuple<float, float, float, float>(73,0,72,97));
		ludioMovesData.push_back(tuple<float, float, float, float>(146, 0, 72,97));
		ludioMovesData.push_back(tuple<float, float, float, float>(0,98,72,97));
		ludioMovesData.push_back(tuple<float, float, float, float>(73,98,72,97));
		ludioMovesData.push_back(tuple<float, float, float, float>(146,98,72,97));
		ludioMovesData.push_back(tuple<float, float, float, float>(219,0,72,97));
		ludioMovesData.push_back(tuple<float, float, float, float>(292,0,72,97));
		ludioMovesData.push_back(tuple<float, float, float, float>(219,97,72,97));
		ludioMovesData.push_back(tuple<float, float, float, float>(365,0,72,97));
		ludioMovesData.push_back(tuple<float, float, float, float>(292,98,72,97));

		//for enemy animations
		//fly 2 
		//glop
		//snail 2
		enemiesAnimData.push_back(tuple<float, float, float, float>(0, 32, 72, 36));
		enemiesAnimData.push_back(tuple<float, float, float, float>(0, 0, 75, 31));
		enemiesAnimData.push_back(tuple<float, float, float, float>(52, 125, 50, 28));
		enemiesAnimData.push_back(tuple<float, float, float, float>(0, 125, 51, 26));
		enemiesAnimData.push_back(tuple<float, float, float, float>(143, 34, 54, 31));
		enemiesAnimData.push_back(tuple<float, float, float, float>(67, 87, 57, 31));
		//for the background snow platform
		GLuint platformSheet = LoadTexture("tiles_spritesheet.png");
		//for the player animations
		ludioSheet = LoadTexture("p1_spritesheet.png");
		//for enemies
		enemiesSheet = LoadTexture("enemies_spritesheet.png");

		Entity ludio; //player
		
		vector<Entity> enemies; //enemies
		
		//setUp
		setUp(platform,platformSheet, ludio, ludioSheet, enemies, enemiesSheet);

		


		enum GameState {TITLE_SCREEN, GAME_STATE, GAME_OVER};
		GameState state = TITLE_SCREEN;
		win = false;

		glViewport(0, 0, 640, 360);
		glEnable(GL_BLEND);	
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
		Matrix modelMatrix;
		Matrix projectionMatrix;
		Matrix viewMatrix;
		projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
		glUseProgram(program.programID);

		float lastTick = 0.0f;

	SDL_Event event;
	bool done = false;
	while (!done) {
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastTick;
		lastTick = ticks;

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					//player starts game
					if (state == TITLE_SCREEN) {
						state = GAME_STATE;						
					}
				}
				//restart 
				if (event.key.keysym.scancode == SDL_SCANCODE_R && state == GAME_OVER) {
					state = TITLE_SCREEN;
					//reset everything
					win = false;
					
					setUp(platform, platformSheet, ludio, ludioSheet, enemies, enemiesSheet);
				}

			}
		}

		glClear(GL_COLOR_BUFFER_BIT);
		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		float vertices[] = { -1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1 };
		float textCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };

		switch (state) {
			case TITLE_SCREEN:
				//Title
				modelMatrix.identity();
				modelMatrix.Translate(-1.75f, 1.5f, 0.0f);
				program.setModelMatrix(modelMatrix);
				DrawText(&program, font, "Space Inavders", 0.25f, 0);
				//Instructions
				modelMatrix.identity();
				modelMatrix.Translate(-3.0f, 1.0f, 0.0f);
				program.setModelMatrix(modelMatrix);
				DrawText(&program, font, "Instructions:", 0.25f, 0);
				modelMatrix.identity();
				modelMatrix.Translate(-2.75f, 0.75f, 0.0f);
				program.setModelMatrix(modelMatrix);
				DrawText(&program, font, "Arrow keys to move", 0.25f, 0);
				modelMatrix.identity();
				modelMatrix.Translate(-2.75f, 0.50f, 0.0f);
				program.setModelMatrix(modelMatrix);
				DrawText(&program, font, "Space to shoot", 0.25f, 0);
				//Game On
				modelMatrix.identity();
				modelMatrix.Translate(-2.50f, -0.5f, 0.0f);
				program.setModelMatrix(modelMatrix);
				DrawText(&program, font, "Press Space to Play", 0.25f, 0);
			break;

			//play
			case GAME_STATE:
				
				//update
				ludio.update(elapsed);
				for (Entity& e1 : enemies) {
					//e1.update(elapsed);
				}
				//draw
				for (Entity& pl : platform) {
					pl.draw(&program);
				}
				ludio.draw(&program);

				for (Entity& e1 : enemies) {
					e1.draw(&program);
				}

				break;

			case GAME_OVER:
				modelMatrix.identity();
				modelMatrix.Translate(-1.75f, 1.5f, 0.0f);
				program.setModelMatrix(modelMatrix);
				string toWrite = "";
				//Did the player win, did the player lose
				if (win) {
					toWrite = "YOU WIN";
				}
				else {
					toWrite = "YOU LOSE";
				}
				DrawText(&program, font, toWrite, 0.5f, 0.0f);
				modelMatrix.identity();
				modelMatrix.Translate(-2.5f, 0.0f, 0.0f);
				program.setModelMatrix(modelMatrix);
				DrawText(&program, font, "Press R to Play Again", 0.25f, 0.0f);
		}

		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);

		SDL_GL_SwapWindow(displayWindow);
	}


	SDL_Quit();
	return 0;
}
