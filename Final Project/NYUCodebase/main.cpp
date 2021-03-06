//Luvneesh Mugrai
//Intro to Game Programming
//Lets Go For a Walk 
//Final Project

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
#include <algorithm>

#include <Windows.h>
#include <tuple>

#include <SDL_mixer.h>

using namespace std;

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

#define FIXED_TIMESTEP 0.01666666f
#define MAX_TIMESTEPS 6

class Vector2 {
public:
	float x;
	float y;
	Vector2() { x = 0.0f; y = 0.0f; }
	Vector2(float x1, float y1) : x(x1), y(y1) {}

	float dist(Vector2 otherDude) {
		return sqrt((x - otherDude.x)*(x - otherDude.x) + (y - otherDude.y)*(y - otherDude.y));
	}
};

class Entity;

//is player playing still
bool win = false;
//the current score
float score = 00.0;
bool drawHighScore = false;

//for player
GLuint ludioSheet;
Vector2 ludionPosVec;

//player animations
int ludioIndex = 0;
float ludioSize = 1.0f;
vector<tuple<float, float, float, float>> ludioMovesData;
//make the game hard
int jumps_left = 5;

//for enemies
vector<Entity> enemiesBasis; //enemies starter 
vector<Entity> enemiesToFight; //real enemies based of basis
GLuint enemiesSheet;
vector<tuple<float, float, float, float>> enemiesAnimData;
int flyIndex = 0;
int glopIndex = 2;
int snailIndex = 4;
float enSize = 0.5f;
//platform
vector<Entity> platform;

enum GameState { TITLE_SCREEN, GAME_STATE, PAUSE, GAME_OVER_DRAW_ONLY };
GameState state;

//sound effects
//jump
Mix_Chunk *jumpSound;
Mix_Chunk *floatSound;
Mix_Chunk *gameOverSound;
Mix_Chunk *highScoreSound;
Mix_Chunk *makeHarder;
Mix_Music *musicMusic;

bool globHarder = true;
bool flyHarder = true;
bool snailHarder = true;

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
	//THIS GOT RID OF MY LINE ISSUE
	//USING GL_NEAREST FOR RENDERING THE TILES 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//also for the background
	//using GL_Repeat allowed for me to wrap around with my uv coords
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	stbi_image_free(image);
	return retTexture;
}

GLuint LoadTextureLinear(const char *filePath) {
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
	SheetSprite(unsigned int xtextureID, float tu, float tv, float twidth, float theight, float tsize, float aw, float ah) {
		u = tu;
		v = tv;
		width = twidth;
		height = theight;
		size = tsize;
		textureID = xtextureID;
		this->aW = aw;
		this->aH = ah;
	}
	SheetSprite() {}
	void Draw(ShaderProgram *program);

	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
	float aW;
	float aH;
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

	float aspect = aW / aH;

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

float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t*v1;
}

class Entity {
	bool left = false;
	bool right = true;
public: 
	Entity() {}

	Entity(float x, float y, float width, float height, string type, float xVel, float yVel, bool alive) {
		this->pos.x = x;
		this->pos.y = y;
		this->width = width;
		this->height = height;
		this->type = type;
		this->veclocityVec.y = yVel;
		this->veclocityVec.x = xVel;
		this->alive = alive;
		aspect = this->width / this->height;
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
				const Uint8 *keys = SDL_GetKeyboardState(NULL);
				//only jump once
				//aka no double jump
				if (!jump) {
					//when jump allowed
					//when jumps_left != 0
					if ((keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W]) && (jumps_left != 0)) {
						if (collidedBottom == true) {
							veclocityVec.y = 4.0f;
							//limited # of jumps
							if (jumps_left > 0) {
								Mix_PlayChannel(-1, jumpSound, 0);
								jumps_left -= 1;
							}
						}
						jump = true;
					}
					//animate walking man
					if (animationElapsed > 1.0f / framesPerSecond) 
					{
						float Swidth = get<2>(ludioMovesData[ludioIndex]);
						float Sheight = get<3>(ludioMovesData[ludioIndex]);
						aspect = Swidth / Sheight;
						sprite = SheetSprite(ludioSheet, get<0>(ludioMovesData[ludioIndex]) / 512.0f,
							get<1>(ludioMovesData[ludioIndex]) / 512.0f, Swidth / 512.0f, Sheight / 512.0f, ludioSize, (Swidth*aspect), (Sheight*aspect));
						ludioIndex = (ludioIndex + 1) % ludioMovesData.size();
						animationElapsed = 0.0f;
					}
				}
				
				//change sprite to jump once
				//while I am in the air 
				//you can insta teleport back
				else if (jump) {
					float Swidth = 67.0f / 512.0f;
					float Sheight = 94.0f / 512.0f;
					aspect = Swidth / Sheight;
					sprite = SheetSprite(ludioSheet, 438.0f / 512.0f,
						93.0f / 512.0f, Swidth, Sheight, ludioSize, (Swidth*aspect), (Sheight*aspect));
					//slowly drop the player velocity cause air friction....and gravity too
					//adding this in makes spamming even harder....
					//veclocityVec.y = lerp(veclocityVec.y, 0.0f, elapsed*frictionVec.y);
				}
				//while I am in the air 
				//you can insta teleport back
				if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S]) {
					pos.y = -0.99f;
					jump = false;
				}
				
				veclocityVec.y += elapsed * gravity;
				//limit to how high you can jump
				//dont go off my screen
				if (pos.y > 1.4f) {
					veclocityVec.y = -0.1f; //no higher than this
				}

				pos.y += veclocityVec.y * elapsed;
				//cause technically player not moving in x
				//just check y collision with ground
				//and also check enemies
				collideWithGroundY();
				
				//THIS WORKS, COMMENTED OUT FOR NOW TO ADD TO GAME PLAY
				//AD THIS BACK IN LATER TO LOSE GAME
				for (Entity& e : enemiesToFight) {
					collideWithEntity(e);
				}
			}
			//curr entity = enemies
			else {
				//fly enemies
				//Has elements of AI
				//It tracks the players y position 
				//slowly moves towards player's y 
				if (type == "fly") {
					if (animationElapsed > 1.0f / framesPerSecond) {
						float Swidth = get<2>(enemiesAnimData[flyIndex]);
						float Sheight = get<3>(enemiesAnimData[flyIndex]);
						aspect = Swidth / Sheight;
						sprite = SheetSprite(enemiesSheet, get<0>(enemiesAnimData[flyIndex]) / 353.0f,
							get<1>(enemiesAnimData[flyIndex]) / 153.0f, Swidth / 353.0f,
							Sheight / 153.0f, enSize, (Swidth*aspect*(153.0f / 353.0f)), (Sheight*aspect*(153.0f / 353.0f)));
						flyIndex = (flyIndex + 1) % 2;
						animationElapsed = 0.0f;
					}
					//fly move towards ludio's y position
					//just at a really slow rate
					float offsetToPlayer = pos.y - ludionPosVec.y;
					pos.y -= offsetToPlayer * 0.1 * elapsed;
				}
				//glop anim
				if (type == "glop") {
					if (animationElapsed > 1.0f / framesPerSecond) {
						float Swidth = get<2>(enemiesAnimData[glopIndex]);
						float Sheight = get<3>(enemiesAnimData[glopIndex]);
						aspect = Swidth / Sheight;
						sprite = SheetSprite(enemiesSheet, get<0>(enemiesAnimData[glopIndex]) / 353.0f,
							get<1>(enemiesAnimData[glopIndex]) / 153.0f, Swidth / 353.0f,
							Sheight / 153.0f, enSize, (Swidth*aspect*(153.0f / 353.0f)), (Sheight*aspect*(153.0f / 353.0f)));
						animationElapsed = 0.0f;
						glopIndex = ((glopIndex + 1) % 2) + 2;
					}
					
					//fall down to ground
					veclocityVec.y += gravity * elapsed;
					pos.y += veclocityVec.y *elapsed;
					collideWithGroundY();
				}
				if (type == "snail") {
					if (animationElapsed > 1.0f / framesPerSecond) {
						float Swidth = get<2>(enemiesAnimData[snailIndex]);
						float Sheight = get<3>(enemiesAnimData[snailIndex]);
						aspect = Swidth / Sheight;
						sprite = SheetSprite(enemiesSheet, get<0>(enemiesAnimData[snailIndex]) / 353.0f,
							get<1>(enemiesAnimData[snailIndex]) / 153.0f, Swidth / 353.0f,
							Sheight / 153.0f, enSize, (Swidth*aspect*(153.0f / 353.0f)), (Sheight*aspect*(153.0f / 353.0f)));
						snailIndex = ((snailIndex + 1) % 2) + 4;
						animationElapsed = 0.0f;
					}
					veclocityVec.y += gravity * elapsed;
					pos.y += veclocityVec.y *elapsed;
					collideWithGroundY();
					//be smart my ai snail
					//find vec 2 dist to player
					//if below a certain value and you to right 
					//then slow down and mke game harder by throwing off timings
					//activate only score > ###
					if (score > 300 && pos.x > ludionPosVec.x && !slowDownOnce) {
						float linDist = pos.dist(ludionPosVec);
						if (linDist < 0.90f) {
							oldVelVec = veclocityVec;
							veclocityVec.x = veclocityVec.x * 0.75;
							slowDownOnce = true;
						}
						
					}
					//swap back once
					else if (slowDownOnce && pos.x < ludionPosVec.x){
						slowDownOnce = true;
						veclocityVec = oldVelVec;
					}
					
					
				}
				//progressively get faster from their based/modified stats
				if (veclocityVec.x <= 5.0f) {
					veclocityVec.x += accelerationVec.x * elapsed;
				}
				//move in x
				//for all enemies
				pos.x -= veclocityVec.x * elapsed;
				//they have done their job and failed
				offScreen();
			}
		}
	}

	//entity now off screen
	//it served it purpose
	void offScreen() {
		if (this->pos.x + width / 2 < -3.60f) {
			alive = false;
		}
	}

	//only will be called for between player and enemies
	//smaller than normal hitbox cause....
	void collideWithEntity(Entity& enem) {
		//I collided with something to my right
		//but it is did not fully pass me 
		if (this->pos.x + width / 3 > enem.pos.x - enem.width / 3 &&
			this->pos.x < enem.pos.x) {
			if (this->pos.y - height / 3 < enem.pos.y + enem.height / 3 && this->pos.y + height / 3 > enem.pos.y - height / 3) {
				
				win = false; 
				Mix_PlayChannel(-1, gameOverSound, 0);
				state = GAME_OVER_DRAW_ONLY;
			//	return true;
			}
		}
	}

	//collided with platform
	void collideWithGroundY() {
		//check with only 1st row 
		//1st row all same so check any of the 15 y
			if (this->pos.y-height/2 < platform[0].pos.y+platform[0].height/2-0.25) {
				collidedBottom = true;
				veclocityVec.y = 0.0f;
				accelerationVec.y = 0.0f;
				pen_y = (platform[0].height/2 + platform[0].pos.y-0.25) - (pos.y-height/2);
				pos.y += pen_y + 0.002;
				jump = false;
			}

			else {
				collidedBottom = false;
				//acceleration_y = -3.7;
				pen_y = 0;
			}	
	}
	
	void draw(ShaderProgram *program) {
		if (alive) {
			modelMatrix.identity();
			modelMatrix.Translate(pos.x, pos.y, 0);
			program->setModelMatrix(modelMatrix);
			sprite.Draw(program);
		}
	}

	string type;
	Vector2 veclocityVec = Vector2(0.0f, 0.0f);
	Vector2 oldVelVec = veclocityVec;
	/*float xVel;
	float yVel = 0.0f;*/

	Vector2 pos = Vector2(0,0);

	Vector2 accelerationVec = Vector2(0, 0);
	float gravity = -2.5f;

	Vector2 frictionVec = Vector2(0.5f, 0.5f);

	float width;
	float height;

	float aspect;
	
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
	
	//one time slow down snail
	bool slowDownOnce = false;

	void entityToString() {
		ostringstream ss;
		ss << this->type << " stats: centerX " << pos.x << " centerY " << pos.y << " width " << width << " height " << height
			<< " speedX " << veclocityVec.x << " speedY" << veclocityVec.y;
		//OutputDebugString(ss.str().c_str());
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
	float platformY = -1.2f;
	for (int i = 0; i < 15; i++) {
		Entity platformT  = Entity(platformX, platformY, 0.5*platSize*platAspect * 2, 0.5f*platSize * 2, "platformTop", 0.0f, 0.0f, true);
		platformT.sprite = SheetSprite(platSheet, 144.0f / 1024.0f, 792.0f / 1024.0f, platW / 1024.0f, platH / 1024.0f, platSize, platW / 1024.0f, platH / 1024.0f);
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
		platformT.sprite = SheetSprite(platSheet, 720.0f / 1024.0f, 864.0f / 1024.0f, platW / 1024.0f, platH / 1024.0f, platSize, platW / 1024.0f, platH / 1024.0f);
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
	float playerY = -0.20f;
	player = Entity(playerX, playerY, 0.5*playerSize*playerAspect * 2.0f, 0.5f*playerSize*2.0f, "player", 0.0f, 0.0f, true);
	player.sprite = SheetSprite(playerSheet, 67.0f / 512.0f, 196.0f / 512.0f, playerW / 512.0f, playerH / 512.0f, playerSize, playerW / 512.0f, playerH / 512.0f);
	//enemies
	//starting 
	//1 of each
	//fly walk 1
	float flySize = enSize;
	float flyW = 72.0f;
	float flyH = 36.0f; 
	float flyAsp = flyW / flyH;
	float flyX = 4.25f;
	float flyY = 1.2f;
	Entity fly = Entity(flyX, flyY, 0.5*flySize*flyAsp * 2, 0.5f*flySize * 2, "fly", 1.750f, 0.0f, true);
	fly.sprite = SheetSprite(eSheets, 0.0f / 353.0f, 32.0f / 153.0f, flyW / 353.0f, flyH / 153.0f, flySize,(flyW*flyAsp*(153.0f/353.0f)),(flyH*flyAsp*(153.0f/353.0f))); //mult flysize by fly aspect and the spritesheet aspect
	e.push_back(fly);
	//glop walk 
	float glopSize = enSize;
	float glopW = 50.0f;
	float glopH = 28.0f;
	float glopAsp = glopW / glopH;
	float glopX = 4.25f;
	float glopY = -0.70f;
	Entity glop = Entity(glopX, glopY, 0.5*glopSize*glopAsp * 2, 0.5f*glopSize * 2, "glop", 2.1f, 0.0f, true);
	glop.sprite = SheetSprite(eSheets, 52.0f / 353.0f, 125.0f / 153.0f, glopW / 353.0f, glopH / 153.f, glopSize, (glopW*glopAsp*(153.0f / 353.0f)), (glopH*glopAsp*(153.0f / 353.0f)));
	e.push_back(glop);
	//snail walk 1
	float snailSize = enSize;
	float snailW = 54.0f;
	float snailH = 31.0f;
	float snailAsp = snailW / snailH;
	float snailX = 4.25f;
	float snailY = -0.70f;
	Entity snail = Entity(snailX, snailY, 0.5*snailSize*snailAsp * 2, 0.5f*snailSize * 2, "snail", 1.50f, 0.0f, true);
	snail.sprite = SheetSprite(eSheets, 143.0f / 353.0f, 34.0f / 153.0f, snailW / 353.0f, snailH / 153.f, snailSize, (snailW*snailAsp*(153.0f / 353.0f)), (snailH*snailAsp*(153.0f / 353.0f)));
	e.push_back(snail);
}

//render the new enemies
//randomly procedurally generated
void renderEnemy(vector<Entity>& list, vector<Entity>& enemiesShow, bool holdup) {
	stringstream ss;
	int spawnPick = rand() % 30; //range: 0 to 29
	if (spawnPick < 10) { //snail
		enemiesShow.push_back(list[2]);
		ss << "snail ";
		if (holdup) {
			enemiesShow.back().pos.x += (enemiesShow.back().width * 2);
		}
	}
	else if (spawnPick < 20 && score > 400) { //glop
		enemiesShow.push_back(list[1]);
		ss << "glop ";
		if (globHarder) {
			Mix_PlayChannel(-1, makeHarder, 0);
			globHarder = false;
		}
		if (holdup) {
			enemiesShow.back().pos.x += (enemiesShow.back().width * 2);
			
		}
	}
	else if (spawnPick < 30 && score > 150) { //fly
		enemiesShow.push_back(list[0]);
		ss << "fly ";
		if (flyHarder) {
			Mix_PlayChannel(-1, makeHarder, 0);
			flyHarder = false;
		}
		if (holdup) {
			enemiesShow.back().pos.x += (enemiesShow.back().width * 2);
		}
	}
}

//PURGE
void purge() {
	enemiesToFight.erase(
		std::remove_if(enemiesToFight.begin(), enemiesToFight.end(), [](Entity& p) {return !p.alive;}
			),
		enemiesToFight.end()
		);
}

//draw the moving background
//using GL_Repeat allowed for me to wrap around with my uv coords
void drawBG(ShaderProgram* program, GLuint img, Matrix modelMatrix, float offSet) {
	glBindTexture(GL_TEXTURE_2D, img);
	float vertices[] = /*{ -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };*/{ -1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1 };//{ -0.125, -0.5, 0.125, -0.5, 0.125, 0.5, -0.125, -0.5, 0.125, 0.5, -0.125, 0.5 };
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	//show all of image
	float textCoords[] = { 0.0 + offSet, 1.0,
						   1.0 + offSet, 1.0,
						   1.0 + offSet, 0.0,
						   0.0 + offSet, 1.0,
						   1.0 + offSet, 0.0,
						   0.0 + offSet, 0.0};
	//float textCoords[] = { 0.0, 1.0, 2.0, 1.0, 2.0, 0.0, 0.0, 1.0, 2.0, 0.0, 0.0, 0.0 };
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, textCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	program->setModelMatrix(modelMatrix);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

//draw jumps left
void drawJumpIcon(ShaderProgram* program, GLuint img, Matrix modelMatrix) {
	glBindTexture(GL_TEXTURE_2D, img);
	float vertices[] = { -0.1, -0.1, 0.1, -0.1, 0.1, 0.1, -0.1, -0.1, 0.1, 0.1, -0.1, 0.1 };//{ -1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1 };//{ -0.125, -0.5, 0.125, -0.5, 0.125, 0.5, -0.125, -0.5, 0.125, 0.5, -0.125, 0.5 };
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	//show all of image
	float textCoords[] = { 0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0,
		0.0, 1.0,
		1.0, 0.0,
		0.0, 0.0 };
	//float textCoords[] = { 0.0, 1.0, 2.0, 1.0, 2.0, 0.0, 0.0, 1.0, 2.0, 0.0, 0.0, 0.0 };
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, textCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	program->setModelMatrix(modelMatrix);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

//update moving alive entities
//entity update
void updateAll(float elapsed, Entity& ludio) {
	//update
	ludio.update(elapsed);
	//get ludio's new position
	ludionPosVec = ludio.pos;
	/*ludioY = ludio.pos.y;
	ludioX = ludio.pos.x;*/
	for (Entity& e1 : enemiesToFight) {
		e1.update(elapsed);
	}
}

//this time for real
//took main update stuff and dropped it here
//update for basically everything else needs time
void updateAllForReal(float elapsed, float& scoreElapsed, float& renderEnemyNow, Entity& ludio, float& recoverJump) {
	//score
	scoreElapsed += elapsed;
	if (scoreElapsed > 1.0f / 10.0f) {
		score++;
		scoreElapsed = 0.0f;
		if (score > 300 && snailHarder) {
			Mix_PlayChannel(-1, makeHarder, 0);
			snailHarder = false;
		}
	}

	//procedurally generated enemies
	//kind of.....
	//render enemy function
	renderEnemyNow += elapsed;
	if (renderEnemyNow > 2.0f) {
		renderEnemy(enemiesBasis, enemiesToFight, false);
		//80% chance of rendering extra enemy every 2 secs
		if (score > 700 && rand() % 10 < 8) {
			renderEnemy(enemiesBasis, enemiesToFight, true);
		}
		//50% chance of rendering extra enemy every 2 secs
		else if (score > 350 && rand() % 10 < 5) {
			renderEnemy(enemiesBasis, enemiesToFight, true);
		}
		renderEnemyNow = 0.0f;
	}

	//can I recover my jump now??
	recoverJump += elapsed;
	if (recoverJump > 5.0f) {
		if (jumps_left < 5) {
			jumps_left++;
		}
		recoverJump = 0.0f;
	}

	//update my entities
	updateAll(elapsed, ludio);
}

//draw everything
void drawAll(float& bgOffSet, Matrix& modelMatrix, float elapsed, GLuint background, GLuint font, ShaderProgram* program, Entity& ludio, GLuint jumpIcon, float& timeToShow) {
	//draw
	//background first
	bgOffSet += (1.0f / 16.0f) * elapsed;
	modelMatrix.identity();
	modelMatrix.Scale(3.75f, 2.0f, 1.0f);
	drawBG(program, background, modelMatrix, bgOffSet);
	//entities
	//platofrms drawn
	for (Entity& pl : platform) {
		pl.draw(program);
	}
	//ludio is drawn
	ludio.draw(program);
	//render the enemies
	for (Entity& e1 : enemiesToFight) {
		e1.draw(program);
	}
	//score 
	modelMatrix.identity();
	modelMatrix.Translate(0.50f, -1.50f, 0.0f);
	program->setModelMatrix(modelMatrix);
	stringstream finalScore;
	finalScore << "Score:" << score;
	DrawText(program, font, finalScore.str(), 0.350f, -0.10f);
	//number of jumps
	modelMatrix.identity();
	modelMatrix.Translate(-3.0f, -1.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	stringstream num_of_jumps;
	num_of_jumps << "Jumps Left";
	DrawText(program, font, num_of_jumps.str(), 0.350f, -.15f);
	float xJumpIconOffset = -2.5f;
	for (int i = 0; i < jumps_left; i++) {
		modelMatrix.identity();
		modelMatrix.Translate(xJumpIconOffset, -1.8f, 0);
		xJumpIconOffset += 0.25f;
		program->setModelMatrix(modelMatrix);
		//thanks for the jump icon Henry Zhou
		drawJumpIcon(program, jumpIcon, modelMatrix);
	}
	if (drawHighScore && timeToShow < 2.0f) {
		//draw you got highscore for like 3 secs or so
		//score 
		modelMatrix.identity();
		modelMatrix.Translate(-2.550f, 1.00f, 0.0f);
		program->setModelMatrix(modelMatrix);
		stringstream youGotHighScore;
		youGotHighScore << "NEW HIGHSCORE!";
		DrawText(program, font, youGotHighScore.str(), 0.50f, -0.10f);
	}
	else {
		drawHighScore = false; 
		timeToShow = 0.0f;
	}

}

//soft reset
void reset(Entity& ludio, GLuint playerSheet) {
	win = false;
	score = 0.0f;
	
	float playerSize = 1.0;
	float playerW = 66.0f;
	float playerH = 92.0f;
	float playerAspect = playerW / playerH;
	float playerX = -2.0f;
	float playerY = -0.20f;
	ludio = Entity(playerX, playerY, 0.5*playerSize*playerAspect * 2.0f, 0.5f*playerSize*2.0f, "player", 0.0f, 0.0f, true);
	ludio.sprite = SheetSprite(playerSheet, 67.0f / 512.0f, 196.0f / 512.0f, playerW / 512.0f, playerH / 512.0f, playerSize, playerW / 512.0f, playerH / 512.0f);

	for (Entity& e : enemiesToFight) {
		e.alive = false;
	}
	purge();
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Let's Go For A Walk", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

		Entity ludio; //player
				
		//for the background snow platform
		GLuint platformSheet = LoadTexture("tiles_spritesheet.png");
		//the rest
		//font
		GLuint font = LoadTextureLinear("font1.png");
		//for the player animations
		ludioSheet = LoadTextureLinear("p1_spritesheet.png");
		//for enemies
		enemiesSheet = LoadTextureLinear("enemies_spritesheet.png");

		GLuint background = LoadTexture("bg_grasslands.png");

		GLuint jumpIcon = LoadTexture("up.png");
				
		//for player animations
		ludioMovesData.push_back(tuple<float, float, float, float>(0, 0, 72, 97));
		ludioMovesData.push_back(tuple<float, float, float, float>(73, 0, 72, 97));
		ludioMovesData.push_back(tuple<float, float, float, float>(146, 0, 72, 97));
		ludioMovesData.push_back(tuple<float, float, float, float>(0, 98, 72, 97));
		ludioMovesData.push_back(tuple<float, float, float, float>(73, 98, 72, 97));
		ludioMovesData.push_back(tuple<float, float, float, float>(146, 98, 72, 97));
		ludioMovesData.push_back(tuple<float, float, float, float>(219, 0, 72, 97));
		ludioMovesData.push_back(tuple<float, float, float, float>(292, 0, 72, 97));
		ludioMovesData.push_back(tuple<float, float, float, float>(219, 97, 72, 97));
		ludioMovesData.push_back(tuple<float, float, float, float>(365, 0, 72, 97));
		ludioMovesData.push_back(tuple<float, float, float, float>(292, 98, 72, 97));

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
				
		//setUp
		setUp(platform,platformSheet, ludio, ludioSheet, enemiesBasis, enemiesSheet);

		state = TITLE_SCREEN;
		win = false;
		//high score
		float highscore = 0;
		bool newHighScore = false;
		 

		glViewport(0, 0, 1280, 720);
		glEnable(GL_BLEND);	
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
		Matrix modelMatrix;
		Matrix projectionMatrix;
		Matrix viewMatrix;
		projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
		glUseProgram(program.programID);

		float lastTick = 0.0f;
		float scoreElapsed = 0.0f;
		float renderEnemyNow = 0.0f;
		float recoverJump = 0.0f;
		float bgOffSet = 0.0f;

		//player cant hold space to fly
		//they have to break the keyboard and spam it 
		bool letThereBeFlight = true;

		float highScoreTime = 0.0;

		//music + sound effects
		Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

		//https://www.freesound.org/people/Cabeeno%20Rossley/
		jumpSound = Mix_LoadWAV("jumpSound.wav");
		floatSound = Mix_LoadWAV("floatSound.wav");
		gameOverSound = Mix_LoadWAV("gameOver.wav");
		//https://www.freesound.org/people/psychentist/sounds/168568/
		highScoreSound = Mix_LoadWAV("highScoreSound.wav");
		makeHarder = Mix_LoadWAV("harder.wav");
		//All credit goes to Manaka Kataoka, Yasuaki Iwata, and Hajime Wakai for creating this wonderful soundtrack.
		musicMusic = Mix_LoadMUS("musicMusic.mp3");
		

		Mix_PlayMusic(musicMusic, -1);

	SDL_Event event;
	bool done = false;
	while (!done) {
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastTick;
		lastTick = ticks;

		float fixedElapsed = elapsed;
		
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					//player starts gameMix_PlayChannel
					if (state == TITLE_SCREEN) {
						state = GAME_STATE;						
					}
					if (state == GAME_STATE) {
						//spam
						//MORE TESTING NEEDS TO BE DONE WITH THE VALUE
						if (letThereBeFlight) {
							//Mix_PlayChannel(-1, floatSound, 0);
							ludio.veclocityVec.y += -0.1495*ludio.gravity;
							letThereBeFlight = false;
						}
					}
				}
				//restart 
				if (event.key.keysym.scancode == SDL_SCANCODE_R && state == GAME_OVER_DRAW_ONLY) {
					state = TITLE_SCREEN;
					//reset everything
					float lastTick = 0.0f;
					float scoreElapsed = 0.0f;
					float renderEnemyNow = 0.0f;
					float recoverJump = 0.0f;
					float bgOffSet = 0.0f;
					jumps_left = 5;
					newHighScore = false;
					drawHighScore = false;
					globHarder = true;
					flyHarder = true;
					snailHarder = true;
					reset(ludio, ludioSheet);
				}
				//pause
				if (event.key.keysym.scancode == SDL_SCANCODE_P && state == GAME_STATE) {
					state = PAUSE;
				}
				//paused - rage quit
				if (event.key.keysym.scancode == SDL_SCANCODE_Q && state == PAUSE) {
					done = true;
				}
				//oaused - resume
				if (event.key.keysym.scancode == SDL_SCANCODE_R && state == PAUSE) {
					state = GAME_STATE;
				}
			}
			else if (event.type == SDL_KEYUP) {
				//you have to spam to float
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					letThereBeFlight = true;
				}
			}
		}

		glClear(GL_COLOR_BUFFER_BIT);
		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		float vertices[] = { -1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1 };
		float textCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };

		switch (state) {
		case TITLE_SCREEN: 
		{
			//Title
			modelMatrix.identity();
			modelMatrix.Translate(-2.0f, 1.5f, 0.0f);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, "Lets Go For A Walk", 0.37f, -0.15f);
			//Instructions
			modelMatrix.identity();
			modelMatrix.Translate(-3.0f, 1.0f, 0.0f);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, "Instructions:", 0.25f, -0.10f);
			modelMatrix.identity();
			modelMatrix.Translate(-2.75f, 0.75f, 0.0f);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, "W/Up to Jump", 0.25f, -0.10f);
			modelMatrix.identity();
			modelMatrix.Translate(-2.75f, 0.50f, 0.0f);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, "S/Down to drop", 0.25f, -0.10f);
			modelMatrix.identity();
			modelMatrix.Translate(-2.750f, 0.25f, 0.0f);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, "Spam! Space to float", 0.25f, -0.10);
			//pause
			modelMatrix.identity();
			modelMatrix.Translate(-2.750f, 0.0f, 0.0f);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, "P to pause", 0.25f, -0.10f);
			//Game On
			modelMatrix.identity();
			modelMatrix.Translate(-2.50f, -0.5f, 0.0f);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, "Press Space to Play", 0.25f, -0.10f);
			//Current HighScore
			modelMatrix.identity();
			modelMatrix.Translate(-3.0f, -1.750f, 0.0f);
			program.setModelMatrix(modelMatrix);
			stringstream theCurrentHighScore;
			theCurrentHighScore << "High Score: " << highscore;
			DrawText(&program, font, theCurrentHighScore.str(), 0.350f, 0.0f);
		}break;

			//play
		case GAME_STATE:
		{
			//play highscore music one time
			//for every game
			if (score > highscore && newHighScore == false && score > 100) {
				newHighScore = true;
				Mix_PlayChannel(-1, highScoreSound, 0);
				//tell player he got the high score
				//so draw it for like 3 secs
				drawHighScore = true;
			}
			highscore = score > highscore ? score : highscore;
			
			//fixed updating
			if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
				fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
			}
			while (fixedElapsed >= FIXED_TIMESTEP) {
				fixedElapsed -= FIXED_TIMESTEP;
				updateAllForReal(fixedElapsed, scoreElapsed, renderEnemyNow, ludio, recoverJump);
			}
			updateAllForReal(fixedElapsed, scoreElapsed, renderEnemyNow, ludio, recoverJump);
			if (drawHighScore) highScoreTime += fixedElapsed;
			//draw
			drawAll(bgOffSet, modelMatrix, fixedElapsed, background, font, &program, ludio, jumpIcon, highScoreTime);

			//PURGE...
			if (enemiesToFight.size() != 0) {
				purge();
			}
		}break;

		case PAUSE:
		{
			//draw
			for (Entity& pl : platform) {
				pl.draw(&program);
			}
			ludio.draw(&program);

			for (Entity& e1 : enemiesToFight) {
				e1.draw(&program);
			}
			//purge...
			if (enemiesToFight.size() != 0) {
				purge();
			}
			modelMatrix.identity();
			modelMatrix.Translate(-2.0f, 1.5f, 0.0f);
			program.setModelMatrix(modelMatrix);
			string pWrite = "";
			//Did the player win, did the player lose
			pWrite = "PAUSED";
			DrawText(&program, font, pWrite, 0.37f, 0.0f);
			modelMatrix.identity();
			modelMatrix.Translate(-3.20f, 1.25f, 0.0f);
			program.setModelMatrix(modelMatrix);
			//Did the player win, did the player lose
			pWrite = "R to resume";
			DrawText(&program, font, pWrite, 0.3f, 0.0f);
			modelMatrix.identity();
			modelMatrix.Translate(-3.20f, 1.00f, 0.0f);
			program.setModelMatrix(modelMatrix);
			//Did the player win, did the player lose
			pWrite = "Q to exit";
			DrawText(&program, font, pWrite, 0.3f, 0.0f);
		}break;

		//lost, but see how you lost
		case GAME_OVER_DRAW_ONLY: 
		{
			//draw
			for (Entity& pl : platform) {
				pl.draw(&program);
			}
			ludio.draw(&program);

			for (Entity& e1 : enemiesToFight) {
				e1.draw(&program);
			}
			//purge...
			if (enemiesToFight.size() != 0) {
				purge();
			}
			//Game over text
			modelMatrix.identity();
			modelMatrix.Translate(-3.20f, 1.5f, 0.0f);
			program.setModelMatrix(modelMatrix);
			string toWrite = "";
			//Did the player win, did the player lose
			toWrite = "Which way is home?";
			DrawText(&program, font, toWrite, 0.37f, 0.0f);
			modelMatrix.identity();
			modelMatrix.Translate(-2.5f, 0.750f, 0.0f);
			program.setModelMatrix(modelMatrix);
			stringstream finalScore;
			finalScore << "Score:" << score;
			DrawText(&program, font, finalScore.str(), 0.350f, 0.0f);
			modelMatrix.identity();
			modelMatrix.Translate(-2.5f, 0.25f, 0.0f);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, "Press R to Play Again", 0.25f, 0.0f);
		}break;
	}

		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);

		SDL_GL_SwapWindow(displayWindow);
	}

	Mix_FreeChunk(jumpSound);
	Mix_FreeChunk(gameOverSound);
	Mix_FreeChunk(floatSound);
	Mix_FreeMusic(musicMusic);
	SDL_Quit();
	return 0;
}
