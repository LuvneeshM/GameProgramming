//Luvneesh Mugrai
//Intro to Game Programming
//Platformer

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
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

#include <Windows.h>
using namespace std;

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

#define SPRITE_COUNT_X 24
#define SPRITE_COUNT_Y 16
#define TILE_SIZE 0.2f
#define LEVEL_HEIGHT 32
#define LEVEL_WIDTH 128
#define FIXED_TIMESTEP 0.01666666f
#define MAX_TIMESTEPS 6

enum GameState { TITLE_SCREEN, GAME_STATE, GAME_OVER };
GameState state = TITLE_SCREEN;

ShaderProgram *program;
Matrix modelMatrix;
Matrix viewMatrix;

int mapHeight;
int mapWidth;
short **levelData;


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


enum EntityType {Player, Enemy, Point};
GLuint sheet;

float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t*v1;
}

class Entity {
public:
	Entity(){}
	Entity(float xPos, float yPos, int indexP, string typeP) {
		x = xPos;
		y = yPos;
		index = indexP;
		if (typeP == "Player") {
			typeE = Player;
			friction_x = 0.25f;
			acceleration_x = 0;
		}
		else if (typeP == "Enemy") {
			typeE = Enemy;
			friction_x = 0.5f;
			acceleration_x = 1.0f;
		}
		else if (typeP == "Point") {
			typeE = Point;
			friction_x = 0;
			friction_y = 0;
		}
	}

	void Update(float elapsed) {
		velocity_x = lerp(velocity_x, 0.0f, elapsed*friction_x);
		velocity_y = lerp(velocity_y, 0.0f, elapsed*friction_y);
		if (typeE == Player) {
			const Uint8 *keys = SDL_GetKeyboardState(NULL);
			if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) {
				acceleration_x = -0.5f;
			}
			else if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) {
				acceleration_x = 0.5f;
			}
			if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W]) {
				if (collidedBottom == true) velocity_y = 3.0f;
			}
		}
		if(velocity_x < 10.0f)
			velocity_x += acceleration_x * elapsed; 
		velocity_y += acceleration_y * elapsed;

		x += velocity_x * elapsed;
		y += velocity_y * elapsed;

	}

	void Render(ShaderProgram *program) {
		if (life) {
			float u = (float)((index) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
			float v = (float)((index) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;

			float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
			float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;

			GLfloat textCoords[] = {
				u, v + spriteHeight,
				u + spriteWidth, v,
				u, v,
				u + spriteWidth, v,
				u, v + spriteHeight,
				u + spriteWidth, v + spriteHeight
			};

			float vertices[] = {
				-0.5f*TILE_SIZE, -0.5f*TILE_SIZE,
				0.5f*TILE_SIZE, 0.5f*TILE_SIZE,
				-0.5f*TILE_SIZE, 0.5f*TILE_SIZE,
				0.5f*TILE_SIZE, 0.5f*TILE_SIZE,
				-0.5f*TILE_SIZE, -0.5f*TILE_SIZE,
				0.5f*TILE_SIZE, -0.5f*TILE_SIZE
			};

			ModelMatrix.identity();
			ModelMatrix.Translate(x, y, 0);
			program->setModelMatrix(ModelMatrix);

			glUseProgram(program->programID);

			glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
			glEnableVertexAttribArray(program->positionAttribute);
			glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, textCoords);
			glEnableVertexAttribArray(program->texCoordAttribute);

			glBindTexture(GL_TEXTURE_2D, sheet);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			glDisableVertexAttribArray(program->positionAttribute);
			glDisableVertexAttribArray(program->texCoordAttribute);

			
		}
	}

	

	void colldeWithEntity(Entity *entity) {
		if (entity->typeE == Enemy) {
			life = collision(entity) ? false : true;
			state = life ? state : GAME_OVER;
		}
		if (entity->typeE == Point) {
			win = collision(entity) ? true : false;
			state = win ? GAME_OVER : state;
		}
	}
	void colldeWithTile() {
		//entity and tiles
		int tileX = 0; 
		int tileY = 0;

		//bot
		worldToTileCoordinates(x, y - height / 2, &tileX, &tileY);
		if (solidTile(levelData[tileY][tileX])) {
			collidedBottom = true;
			velocity_y = 0; 
			acceleration_y = 0;
			pen_y = (-TILE_SIZE*tileY) - (y - height / 2);
			y += pen_y + 0.002f;
		}
		else {
			collidedBottom = false;
			acceleration_y = -9.8;
			pen_y = 0;
		}
		//top
		worldToTileCoordinates(x, y + height / 2, &tileX, &tileY);
		if (solidTile(levelData[tileY][tileX])) {
			collidedTop = true;
			velocity_y = 0;
			pen_y = fabs((y + height / 2) - ((-TILE_SIZE*tileY) - TILE_SIZE));
			y -= (pen_y + 0.002f);
		}
		else {
			collidedTop = false;
			pen_y = 0;
		}
		//right
		worldToTileCoordinates(x + width / 2, y, &tileX, &tileY);
		if (solidTile(levelData[tileY][tileX])) {
			collidedRight = true;
			velocity_x = 0;
			if (typeE == Enemy) {
				acceleration_x *= -1.0f;
			}
			pen_x = (TILE_SIZE*tileX) - (x + width / 2); 
			x -= (pen_x + 0.005f);
		}
		else {
			collidedRight = false;
			pen_x = 0;
		}
		//left
		worldToTileCoordinates(x - width / 2, y, &tileX, &tileY);
		if (solidTile(levelData[tileY][tileX])) {
			collidedLeft = false;
			velocity_x = 0; 
			if (typeE == Enemy) {
				acceleration_x *= -1.0f;
			}
			pen_x = (x - width / 2) - (TILE_SIZE*tileX + TILE_SIZE);
			x += (pen_x + 0.005f);
		}
		else {
			collidedLeft = false;
			pen_x = 0;
		}
	}

	void entityToString() {
		ostringstream ss;
		ss << this->typeE << " stats: centerX " << x << " centerY " << y << " width " << width << " height " << height
			<< " speedX " << velocity_x << " speedY" << velocity_y;
		OutputDebugString(ss.str().c_str());
	}

//private:

	bool life = true;
	bool win = false;

	float x, y;
	float height = TILE_SIZE;
	float width = TILE_SIZE;
	int index;
	EntityType typeE;
	
	SheetSprite sprite;
	Matrix ModelMatrix;

	float acceleration_x = 0; 
	float acceleration_y = 0.0f;
	float velocity_x = 0;
	float velocity_y = 0;

	float friction_x;
	float friction_y = 0.5f;

	float pen_y = 0;
	float pen_x = 0;

	bool collidedTop = false;
	bool collidedBottom = false;
	bool collidedLeft = false;
	bool collidedRight = false;

	bool isStatic = false;

private:

	bool solidTile(int index) {
		if (index == 32 || index == 51 || index == 299 || index == 321 || index == 347 || index == 345 ||
			index == 301) {
			return true;
		}
		return false;
	}

	void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
		*gridX = (int)(worldX / TILE_SIZE);
		*gridY = (int)(-worldY / TILE_SIZE);
	}

	bool collision(Entity *entity) {
		bool toReturn = false;
		if (
			(x + width / 2 >= entity->x - entity->width / 2) &&
			(x + width / 2 <= entity->x + entity->width / 2) &&
			(y - height / 2 <= entity->y + entity->height / 2)
			) {
			toReturn = true;
		}
		else if (
			(x - width / 2 >= entity->x - entity->width / 2) &&
			(x - width / 2 <= entity->x + entity->width / 2) &&
			(y - height / 2 <= entity->y + height / 2)
			) {
			toReturn = true;
		}

		return toReturn;
	}
};

//Entities
Entity player;
Entity enemy[3];
int enemyNum = 0;
Entity point;

void centerPlayer() {
	viewMatrix.identity();
	viewMatrix.Scale(2.0f, 2.0f, 1.0f);
	if (player.y <= -5.5f) {
		viewMatrix.Translate(-player.x, 5.5f, 0.0f);
	}
	else { viewMatrix.Translate(-player.x, -player.y, 0.0f); }
	program->setViewMatrix(viewMatrix);
}

//places entity at their start positions
void placeEntity(const string& type, float x, float y) {
	if (type == "Player") {
		player = Entity(x, y, 90, type);
	}
	else if (type == "Enemy") {
		enemy[enemyNum++] = Entity(x, y, 136, type);
	}
	else if (type == "Point") {
		point = Entity(x, y, 146, type);
	}
}

bool readHeader(std::ifstream &stream) {
	string line;
	mapWidth = -1;
	mapHeight = -1;
	while (getline(stream, line)) {
		if (line == "") { break; }

		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		
		if (key == "width") {
			mapWidth = atoi(value.c_str());
		}
		else if (key == "height") {
			mapHeight = atoi(value.c_str());
		}
	}

	if (mapWidth == -1 || mapHeight == -1) {
		return false;
	}
	else { // allocate our map data
		levelData = new short*[mapHeight];
		for (int i = 0; i < mapHeight; ++i) {
			levelData[i] = new short[mapWidth];
		}
		return true;
	}
}

bool readLayerData(std::ifstream &stream) {
	string line;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "data") {
			for (int y = 0; y < mapHeight; y++) {
				getline(stream, line);
				istringstream lineStream(line);
				string tile;

				for (int x = 0; x < mapWidth; x++) {
					getline(lineStream, tile, ',');
					int val = (int)atoi(tile.c_str());
					if (val > 0) {
						// be careful, the tiles in this format are indexed from 1 not 0
						levelData[y][x] = val - 1;
						if (y == 14 && x == 10) {
							ostringstream ss;
							ss << val << " " << y << " " << x << " " << tile.c_str() << " " << (int)levelData[y][x] << "\n";
							OutputDebugString(ss.str().c_str());
						}
					}
					else {
						levelData[y][x] = 0;
					}
				}
			}
		}
	}
	return true;
}

bool readEntityData(std::ifstream &stream) {
	string line;
	string type;

	while (getline(stream, line)) {
		if (line == "") { break; }
	
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		
		if (key == "type") {
			type = value;
		}
		else if (key == "location") {
			istringstream lineStream(value);
			string xPosition, yPosition;
			getline(lineStream, xPosition, ',');
			getline(lineStream, yPosition, ',');
			
			float placeX = atoi(xPosition.c_str())*TILE_SIZE;
			float placeY = atoi(yPosition.c_str())*-TILE_SIZE;
			placeEntity(type, placeX, placeY);
		}
	}
	return true;
}

vector<float> vertexData;
vector<float> texCoordData;
void drawMap() {
	for (int y = 0; y < LEVEL_HEIGHT; y++) {
		for (int x = 0; x < LEVEL_WIDTH; x++) {
			if (levelData[y][x] != 0) {
				float u = (float)(((int)levelData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
				float v = (float)(((int)levelData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;

				float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
				float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;

				vertexData.insert(vertexData.end(), {
					TILE_SIZE * x, -TILE_SIZE * y,
					TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					TILE_SIZE * x, -TILE_SIZE * y,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
				});

				texCoordData.insert(texCoordData.end(), {
					u, v,
					u, v + (spriteHeight),
					u + spriteWidth, v + (spriteHeight),
					u, v,
					u + spriteWidth, v + (spriteHeight),
					u + spriteWidth, v
				});

			}
		}
	}
}

void renderMap() {

	glUseProgram(program->programID);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);

	modelMatrix.identity();
	program->setModelMatrix(modelMatrix);

	glBindTexture(GL_TEXTURE_2D, sheet);
	glDrawArrays(GL_TRIANGLES, 0,vertexData.size()/2);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

void update(float elapsed) {
	//collision will be off player
	//one hit ko only
	player.Update(elapsed);
	player.colldeWithTile();
	for (int i = 0; i < 3; i++) {
		enemy[i].Update(elapsed);
		player.colldeWithEntity(&enemy[i]);
		enemy[i].colldeWithTile();
	}
	player.colldeWithEntity(&point);
	point.Update(elapsed);
	point.colldeWithTile();
}

void render() {
	player.Render(program);
	centerPlayer();

	for (int i = 0; i < 3; i++) {
		enemy[i].Render(program);
	}
	point.Render(program);
}

void init() {

	ifstream infile("map.txt");
	string line;
	while (getline(infile, line)) {
		if (line == "[header]") {
			if (!readHeader(infile)) {
				break;
			}
		}
		else if (line == "[layer]") {
			readLayerData(infile);
		}
		else if (line == "[Object Layer 1]") {
			readEntityData(infile);
		}
	}

	drawMap();
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Platformer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	sheet = LoadTexture("dirt-tiles.png");
	GLuint font = LoadTexture("font1.png");

	glViewport(0, 0, 640, 360);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	program =  new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	Matrix projectionMatrix;
	projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	glUseProgram(program->programID);

	float lastTick = 0.0f;

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
				if (event.key.keysym.scancode == SDL_SCANCODE_R) {
					if (state == GAME_OVER) {
						state = TITLE_SCREEN;
					}
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					if (state == TITLE_SCREEN) {						
						//restart the count on enemey numbers
						enemyNum = 0;

						//reset stuff
						init();
						state = GAME_STATE;
					}
				}
			}
			else if (event.type == SDL_KEYUP) {
				if (event.key.keysym.scancode == SDL_SCANCODE_LEFT || event.key.keysym.scancode == SDL_SCANCODE_A) {
					player.acceleration_x = 0.0f;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT || event.key.keysym.scancode == SDL_SCANCODE_D) {
					player.acceleration_x = 0.0f;
				}
				
			}
		}

		glClearColor(0.75f, 0.75f, 0.75f, 0.5f);
		glClear(GL_COLOR_BUFFER_BIT);

		program->setProjectionMatrix(projectionMatrix);
		program->setViewMatrix(viewMatrix);

		glEnable(GL_BLEND);

		switch (state)
		{
		case TITLE_SCREEN:
			//Title
			modelMatrix.identity();
			modelMatrix.Translate(-1.75f, 1.5f, 0.0f);
			program->setModelMatrix(modelMatrix);
			DrawText(program, font, "Platformers", 0.25f, 0);
			//Instructions
			modelMatrix.identity();
			modelMatrix.Translate(-3.0f, 1.0f, 0.0f);
			program->setModelMatrix(modelMatrix);
			DrawText(program, font, "Instructions:", 0.25f, 0);
			modelMatrix.identity();
			modelMatrix.Translate(-2.75f, 0.75f, 0.0f);
			program->setModelMatrix(modelMatrix);
			DrawText(program, font, "Arrow keys to move", 0.25f, 0);
			modelMatrix.identity();
			modelMatrix.Translate(-2.75f, 0.50f, 0.0f);
			program->setModelMatrix(modelMatrix);
			DrawText(program, font, "Reach the end", 0.25f, 0);
			//Game On
			modelMatrix.identity();
			modelMatrix.Translate(-2.50f, -0.5f, 0.0f);
			program->setModelMatrix(modelMatrix);
			DrawText(program, font, "Press Space to Play", 0.25f, 0);

			//fix the view matrix
			viewMatrix.identity();
			program->setViewMatrix(viewMatrix);
		break;

		case GAME_STATE:
			
			//draw map
			renderMap();
			//fixed updatinge
			
			if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
				fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
			}
			while (fixedElapsed >= FIXED_TIMESTEP) {
				fixedElapsed -= FIXED_TIMESTEP;
				update(FIXED_TIMESTEP);
			}
			//update player
			update(fixedElapsed);

			render();
		break;

		case GAME_OVER:
			modelMatrix.identity();
			modelMatrix.Translate(-1.75f, 1.5f, 0.0f);
			program->setModelMatrix(modelMatrix);
			string toWrite = "";
			//Did the player win, did the player lose
			if (player.win) {
				toWrite = "YOU WIN";
			}
			else {
				toWrite = "YOU LOSE";
			}
			DrawText(program, font, toWrite, 0.5f, 0.0f);
			modelMatrix.identity();
			modelMatrix.Translate(-2.5f, 0.0f, 0.0f);
			program->setModelMatrix(modelMatrix);
			DrawText(program, font, "Press R to Play Again", 0.25f, 0.0f);
			viewMatrix.identity();
			program->setViewMatrix(viewMatrix);
		break;
		}
			

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
