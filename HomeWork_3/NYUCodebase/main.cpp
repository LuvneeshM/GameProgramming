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

bool moveDownOnce = false;
float enemyDIr = 1.0f;
#define MAX_ENEMY 18
float enemyCount = MAX_ENEMY;
bool win = false;

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
	}
	void update(float elapsed) {
		if (alive) {
			if (type == "bullet") {
				if (y >= 2 - height/2 ) //top_screen - 1/2 size
					alive = false;
				y += yVel * elapsed;
				
			}
			if (type == "enemy") {
				
				x += xVel * elapsed* enemyDIr; 
				
			}
		}
	}

	//collision detections
	//I am r1
	//I am enemy
	void checkCollision(Entity& r2) {
		if (!((this->y - this->height / 2 > r2.y + r2.height / 2) ||
			(this->y + this->height / 2 < r2.y - r2.height / 2) ||
			(this->x - this->width / 2 > r2.x + r2.width / 2) ||
			(this->x + this->width / 2 < r2.x - r2.width / 2)
			)) {

			if (r2.type == "bullet") {
				r2.alive = false; 
				this->alive = false;
				enemyCount--;
			}

			if (r2.type == "player") {
				r2.alive = false;
			}
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
	float width;
	float height;
	bool alive;
	Matrix modelMatrix;
	SheetSprite sprite;
	

	void entityToString() {
		ostringstream ss;
		ss << this->type << " stats: centerX " << x << " centerY " << y << " width " << width << " height " << height
			<< " speedX " << xVel << " speedY" << yVel;
		OutputDebugString(ss.str().c_str());
	}
}; //end entity

//set up for the game
//used at start and when player restarts
void setUp(Entity& player, Entity bullets[], vector<Entity>& enemies, const GLuint& sheet) {
	float playerSize = 0.5f;
	float playerAspect = 99.0f / 75.0f;
	player = Entity(0.0f, -1.6f, 0.5*playerSize*playerAspect * 2, 0.5*playerSize * 2, "player", 1.85f, 1.85f, true);
	player.sprite = SheetSprite(sheet, 224.0 / 1024.0f, 832.0 / 1024.0f, 99.0f / 1024.0f, 75.0f / 1024.0f, 0.5f);

	for (int i = 0; i < 3; i++) {
		float size = 0.25f;
		float aspect = 9.0f / 54.0f;
		bullets[i] = Entity(player.x, player.y, 0.5*size*aspect * 2, 0.5*size * 2, "bullet", 1.0f, 1.85f, false); //laserBlue01
		bullets[i].sprite = SheetSprite(sheet, 856 / 1024.0f, 421 / 1024.0f, 9 / 1024.0f, 54 / 1024.0f, size);
	}


	float enemyX = 1.9f;
	float enemyY = 1.5f;
	for (int i = 0; i < MAX_ENEMY/2; i++) { //first row
		float size = 0.5f;
		float aspect = 93.0f / 84.0f;
		Entity em = Entity(enemyX, enemyY, 0.5*size*aspect * 2, 0.5*size * 2, "enemy", .75f, 0.25f, true);
		em.sprite = SheetSprite(sheet, 423 / 1024.0f, 728 / 1024.0f, 93 / 1024.0f, 84 / 1024.0f, 0.35f); //enemyBlue1
		enemies.push_back(em);
		//move over the enemyX pos for each
		enemyX -= 0.6f;
	}
	enemyX = 1.9f;
	enemyY = 1.0f;
	for (int i = 0; i < MAX_ENEMY / 2; i++) { //2nd row
		float size = 0.5f;
		float aspect = 93.0f / 84.0f;
		Entity em = Entity(enemyX, enemyY, 0.5*size*aspect * 2, 0.5*size * 2, "enemy", .75f, 0.25f, true);
		em.sprite = SheetSprite(sheet, 423 / 1024.0f, 728 / 1024.0f, 93 / 1024.0f, 84 / 1024.0f, 0.35); //enemyBlue1
		enemies.push_back(em);
		//move over the enemyX pos for each
		enemyX -= 0.6f;
	}
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
		
		GLuint font = LoadTexture("font1.png");
		GLuint line = LoadTexture("WallSprite.png");
		GLuint sheet = LoadTexture("sheet.png");
		Entity player;
		Entity bullets[3];
		vector<Entity> enemies;
		int bulletIndex = 0;
		setUp(player, bullets, enemies, sheet);

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
		float bulletTime = 0.0f;

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
					//player shoots
					//orginally was going to be three bullets
					//changed to so player only uses 1 bullet
					if (state == GAME_STATE) {
						float bulletElapsed = ticks - bulletTime;
						if (bullets[bulletIndex].alive == false && bulletElapsed > 0.15f) {
							bulletTime = ticks;
							bullets[bulletIndex].x = player.x;
							bullets[bulletIndex].y = player.y + player.height / 2 + bullets[bulletIndex].height / 2;
							bullets[bulletIndex].alive = true;
							//bullets[bulletIndex].entityToString();
							//bulletIndex = (bulletIndex + 1) % 2 ;
						}
					}
					//player starts game
					else if (state == TITLE_SCREEN) {
						state = GAME_STATE;						
					}
				}
				//restart 
				if (event.key.keysym.scancode == SDL_SCANCODE_R && state == GAME_OVER) {
					state = TITLE_SCREEN;
					//reset everything
					win = false;
					enemyCount = MAX_ENEMY;
					enemies.clear();
					setUp(player, bullets, enemies, sheet);
					bulletIndex = 0;
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
				//player controls
				if (keys[SDL_SCANCODE_LEFT]) {
					if (player.x - player.width / 2 > -3.55f)
						player.x -= player.xVel * elapsed;
				}
				else if (keys[SDL_SCANCODE_RIGHT]) {
					if (player.x + player.width / 2 < 3.55f)
						player.x += player.xVel * elapsed;
				}
				//update
				player.update(elapsed);
				for (Entity& b : bullets) {
					b.update(elapsed);
				}
				for (Entity& e : enemies) {
					e.update(elapsed);
					//x-axis penetrations
					if (e.x + e.width / 2 > 3.55) {
						float penetration = fabs(fabs(e.x-3.55) - e.width / 2 - 0.05f);
						for (Entity& ee : enemies) {
								ee.x = ee.x - penetration - 0.003f;
								ee.y -= e.yVel;
								//make the game harder
								if (enemyCount < 3) {
									ee.xVel = 1.35f;
								}
								else if (enemyCount < 5) {
									ee.xVel = 1.25f;
								}
								else if (enemyCount < 7) {
									ee.xVel = 1.05f;
								}
								else if (enemyCount < 9) {
									ee.xVel = 0.95f;
								}
								else if (enemyCount < 11) {
									ee.xVel = 0.85f;
								}
								ee.xVel *= -1;	
						}
					}
					else if (e.x - e.width / 2 < -3.55) {
						float penetration = fabs(fabs(e.x + 3.55) - e.width / 2 - 0.05f);
						for (Entity& ee : enemies) {
							ee.x = ee.x + penetration + 0.003f;
							ee.y -= e.yVel;
							//make the game harder
							if (enemyCount < 2) {
								ee.xVel = -1.35f;
							}
							else if (enemyCount < 3) {
								ee.xVel = -1.25f;
							}
							else if (enemyCount < 4) {
								ee.xVel = -1.05f;
							}
							else if (enemyCount < 5) {
								ee.xVel = -0.95f;
							}
							else if (enemyCount < 6) {
								ee.xVel = -0.85f;
							}
							ee.xVel *= -1;
						}
					}
					if (e.alive) {
						//collision checks
						//enemy player
						e.checkCollision(player);
						//enemy bullet
						for (Entity& b : bullets) {
							if (b.alive) {
								e.checkCollision(b);
							}
						}
						//enemy below lose line
						if (e.y - e.height/2 < -1.4f) {
							win = false; 
							state = GAME_OVER;
						}
					}
				}
				//draw
				player.draw(&program);
				for (Entity& b : bullets) {
					b.draw(&program);
				}
				for (Entity& e : enemies) {
					e.draw(&program);
				}
				
				//boundary line
				glBindTexture(GL_TEXTURE_2D, line);
				glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
				glEnableVertexAttribArray(program.positionAttribute);
				//show all of image
				glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, textCoords);
				glEnableVertexAttribArray(program.texCoordAttribute);
				modelMatrix.identity();
				modelMatrix.Translate(0, -1.2, 0);
				modelMatrix.Scale(3.55, 0.01, 0);
				program.setModelMatrix(modelMatrix);
				glDrawArrays(GL_TRIANGLES, 0, 6);

				//check win lose condtions
				if (enemyCount == 0) {
					win = true;
					state = GAME_OVER;
				}
				if (player.alive == false) {
					win = false;
					state = GAME_OVER;
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
