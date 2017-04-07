//Luvneesh Mugrai
//Intro to Game Programming
//SAT Collisions

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
#include <algorithm>;

#include <Windows.h>
using namespace std;

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

#define FIXED_TIMESTEP 0.01666666f
#define MAX_TIMESTEPS 6

enum GameState { Start_State, GAME_STATE };
GameState state = Start_State;

ShaderProgram *program;

class Vector {
public:
	float x; 
	float y;
	float z = 0;
	float lengthVar;

	Vector() {}
	Vector(float xp, float yp, float zp) : x(xp), y(yp), z(zp) {}

	float length() const{
		return lengthVar;
	}

	void normalize() {
		lengthVar = sqrt(x*x + y*y);
		x /= lengthVar;
		y /= lengthVar;
	}

	Vector operator*(const Matrix& v) {
		Vector vec;

		vec.x = v.m[0][0] * x + v.m[1][0] * this->y + v.m[2][0] * this->z + v.m[3][0] * 1;
		vec.y = v.m[0][1] * x + v.m[1][1] * this->y + v.m[2][1] * this->z + v.m[3][1] * 1;
		vec.z = v.m[0][2] * x + v.m[1][2] * this->y + v.m[2][2] * this->z + v.m[3][2] * 1;

		return vec;
	}

};

bool testSATSeparationForEdge(float edgeX, float edgeY, const std::vector<Vector> &points1, const std::vector<Vector> &points2, Vector &penetration) {
	float normalX = -edgeY;
	float normalY = edgeX;
	float len = sqrtf(normalX*normalX + normalY*normalY);
	normalX /= len;
	normalY /= len;

	std::vector<float> e1Projected;
	std::vector<float> e2Projected;

	for (int i = 0; i < points1.size(); i++) {
		e1Projected.push_back(points1[i].x * normalX + points1[i].y * normalY);
	}
	for (int i = 0; i < points2.size(); i++) {
		e2Projected.push_back(points2[i].x * normalX + points2[i].y * normalY);
	}

	std::sort(e1Projected.begin(), e1Projected.end());
	std::sort(e2Projected.begin(), e2Projected.end());

	float e1Min = e1Projected[0];
	float e1Max = e1Projected[e1Projected.size() - 1];
	float e2Min = e2Projected[0];
	float e2Max = e2Projected[e2Projected.size() - 1];

	float e1Width = fabs(e1Max - e1Min);
	float e2Width = fabs(e2Max - e2Min);
	float e1Center = e1Min + (e1Width / 2.0);
	float e2Center = e2Min + (e2Width / 2.0);
	float dist = fabs(e1Center - e2Center);
	float p = dist - ((e1Width + e2Width) / 2.0);

	if (p >= 0) {
		return false;
	}

	float penetrationMin1 = e1Max - e2Min;
	float penetrationMin2 = e2Max - e1Min;

	float penetrationAmount = penetrationMin1;
	if (penetrationMin2 < penetrationAmount) {
		penetrationAmount = penetrationMin2;
	}

	penetration.x = normalX * penetrationAmount;
	penetration.y = normalY * penetrationAmount;

	return true;
}

bool penetrationSort(const Vector &p1, const Vector &p2) {
	return p1.length() < p2.length();
}

bool checkSATCollision(const std::vector<Vector> &e1Points, const std::vector<Vector> &e2Points, Vector &penetration) {
	std::vector<Vector> penetrations;
	for (int i = 0; i < e1Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e1Points.size() - 1) {
			edgeX = e1Points[0].x - e1Points[i].x;
			edgeY = e1Points[0].y - e1Points[i].y;
		}
		else {
			edgeX = e1Points[i + 1].x - e1Points[i].x;
			edgeY = e1Points[i + 1].y - e1Points[i].y;
		}
		Vector penetration;
		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);
		if (!result) {
			return false;
		}
		penetrations.push_back(penetration);
	}
	for (int i = 0; i < e2Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e2Points.size() - 1) {
			edgeX = e2Points[0].x - e2Points[i].x;
			edgeY = e2Points[0].y - e2Points[i].y;
		}
		else {
			edgeX = e2Points[i + 1].x - e2Points[i].x;
			edgeY = e2Points[i + 1].y - e2Points[i].y;
		}
		Vector penetration;
		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);

		if (!result) {
			return false;
		}
		penetrations.push_back(penetration);
	}

	std::sort(penetrations.begin(), penetrations.end(), penetrationSort);
	penetration = penetrations[0];

	Vector e1Center;
	for (int i = 0; i < e1Points.size(); i++) {
		e1Center.x += e1Points[i].x;
		e1Center.y += e1Points[i].y;
	}
	e1Center.x /= (float)e1Points.size();
	e1Center.y /= (float)e1Points.size();

	Vector e2Center;
	for (int i = 0; i < e2Points.size(); i++) {
		e2Center.x += e2Points[i].x;
		e2Center.y += e2Points[i].y;
	}
	e2Center.x /= (float)e2Points.size();
	e2Center.y /= (float)e2Points.size();

	Vector ba;
	ba.x = e1Center.x - e2Center.x;
	ba.y = e1Center.y - e2Center.y;

	if ((penetration.x * ba.x) + (penetration.y * ba.y) < 0.0f) {
		penetration.x *= -1.0f;
		penetration.y *= -1.0f;
	}

	return true;
}

float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t*v1;
}

class Entity {
public:
	Entity(){}
	Entity(float xScale, float yScale, float roat) {
		scale.x = xScale;
		scale.y = yScale;
		rotation = roat;
	}

	void setVectorCoords(float x1, float y1, float x2, float y2, float x3, float y3,
		float x4, float y4, float x5, float y5, float x6, float y6) {
		vectPoints.push_back(Vector(x1, y1, 0));
		vectPoints.push_back(Vector(x2, y2, 0));
		vectPoints.push_back(Vector(x3, y3, 0));
		vectPoints.push_back(Vector(x4, y4, 0));
		vectPoints.push_back(Vector(x5, y5, 0));
		vectPoints.push_back(Vector(x6, y6, 0));
		projectedAxisPoints = vectPoints;
	}

	void Update(float elapsed, Entity& c1, Entity&c2) {
		rotation += 1.0f * elapsed;
		position.x += accel * elapsed;

		if (position.x < -3.5) { position.x = 3.5; }
		else if (position.x > 3.5f) { position.x = -3.5f; }
		if (position.y < -3.0f) { position.y = 3.54f; }
		else if (position.y > 3.0f) { position.y = -3.54f; }

		ModelMatrix.identity();
		ModelMatrix.Translate(position.x, position.y, 0.0f);
		ModelMatrix.Rotate(rotation);
		ModelMatrix.Scale(scale.x, scale.y, 1.0f);


		//for (int i = 0; i < projectedAxisPoints.size(); i++) {
		//	
		//	
		//	projectedAxisPoints[i] = vectPoints[i] * ModelMatrix;
		//	
		//	////scale * rotate * translate
		//	//// sx 0 
		//	//// 0 sy 
		//	//float eXPos = sx;
		//	//float eYPos = sy;
		//	//// cos -sin 
		//	//// sin cos
		//	// eXPos *= cos(rotation) * vectPoints[i].x - sin(rotation) * vectPoints[i].y;
		//	// eYPos *= sin(rotation) * vectPoints[i].x + cos(rotation) * vectPoints[i].y;
		//	////1 0 tx
		//	////0 1 ty
		//	//eXPos += x;
		//	//eYPos += y;
		//	////save it
		//	//projectedAxisPoints[i].x = eXPos;
		//	//projectedAxisPoints[i].y = eYPos;
		//}
		////collisions
		//collision(c1);
		//collision(c2);
	}

	void collision(Entity& e) {
		for (int i = 0; i < projectedAxisPoints.size(); i++) {

			projectedAxisPoints[i] = vectPoints[i] * ModelMatrix;
			e.projectedAxisPoints[i] = e.vectPoints[i] * e.ModelMatrix;
			////scale * rotate * translate
			//// sx 0 
			//// 0 sy 
			//float eXPos = sx;
			//float eYPos = sy;
			//// cos -sin 
			//// sin cos
			// eXPos *= cos(rotation) * vectPoints[i].x - sin(rotation) * vectPoints[i].y;
			// eYPos *= sin(rotation) * vectPoints[i].x + cos(rotation) * vectPoints[i].y;
			////1 0 tx
			////0 1 ty
			//eXPos += x;
			//eYPos += y;
			////save it
			//projectedAxisPoints[i].x = eXPos;
			//projectedAxisPoints[i].y = eYPos;
		}
		if (checkSATCollision(this->projectedAxisPoints, e.projectedAxisPoints, penetration)) {
			this->position.x += (penetration.x / 2.0);
			this->position.y += (penetration.y / 2.0);
				
			e.position.x -= (penetration.x / 2.0);
			e.position.y -= (penetration.y / 2.0);
				
			this->accel *= -1.0f;
			e.accel *= -1.0f;
			
			//for debug
			//if (talk) {
				entityToString();
				e.entityToString();
			//}
		}
		
	}

	void Render() {
		/*ModelMatrix.identity();
		ModelMatrix.Translate(position.x, position.y, 0.0f);
		ModelMatrix.Rotate(rotation);
		ModelMatrix.Scale(scale.x, scale.y, 1.0f);
*/
		program->setModelMatrix(ModelMatrix);

		float vertices[] = { vectPoints[0].x, vectPoints[0].y, vectPoints[1].x, vectPoints[1].y,
							 vectPoints[2].x, vectPoints[2].y, vectPoints[3].x, vectPoints[3].y,
							 vectPoints[4].x, vectPoints[4].y, vectPoints[5].x, vectPoints[5].y };
		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program->positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program->positionAttribute);
	}

	void entityToString() {
		ostringstream ss;
		ss << name << " " << penetration.length() << " " << penetration.x << " " << penetration.y << " " << accel << endl;
		//ss << "a " << position.x << " " << position.y << endl;
		OutputDebugString(ss.str().c_str());
	}

	Vector scale;
	Vector position; 


	float accel = .50f;
	float rotation;
	Matrix ModelMatrix;

	bool talk = false;
	string name;

	vector<Vector> vectPoints;
	vector<Vector> projectedAxisPoints;
	Vector penetration;

private:

	
};

void collision(Entity& sSq, Entity& mSq, Entity& bSq) {
	sSq.collision(mSq);
	sSq.collision(bSq);
	mSq.collision(bSq);
}

void update(float elapsed, Entity& sSq, Entity& mSq, Entity& bSq) {
	sSq.Update(elapsed, mSq, bSq);
	mSq.Update(elapsed, sSq, bSq);
	bSq.Update(elapsed, sSq, mSq);
	collision(sSq, mSq, bSq);
}

void render(Entity& sSq, Entity& mSq, Entity& bSq) {
	sSq.Render();
	mSq.Render();
	bSq.Render();
}

void init(Entity& sSq, Entity& mSq, Entity& bSq) {
	sSq.setVectorCoords(-0.1, -0.1, 0.1, -0.1, 0.1, 0.1, -0.1, -0.1, 0.1, 0.1, -0.1, 0.1);
	sSq.position.x = -2.0f;
	sSq.position.y = -0.0f;
	sSq.name = "small";
	
	mSq.setVectorCoords(-0.1, -0.1, 0.1, -0.1, 0.1, 0.1, -0.1, -0.1, 0.1, 0.1, -0.1, 0.1);
	mSq.accel *= -1.0;
	mSq.position.x = 0.0f;
	mSq.position.y = 0.0f;
	mSq.name = "med";

	bSq.setVectorCoords(-0.1, -0.1, 0.1, -0.1, 0.1, 0.1, -0.1, -0.1, 0.1, 0.1, -0.1, 0.1);
	bSq.position.x = 2.0f;
	bSq.position.y = 0.25f;
	bSq.name = "big";
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("SAT_Collision", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	glViewport(0, 0, 640, 360);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	program =  new ShaderProgram(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	Matrix projectionMatrix;
	Matrix viewMatrix;
	projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	glUseProgram(program->programID);

	Entity sSq(1.5f, 1.5f, 30 * 3.14159 / 180);
	Entity mSq(2.25f, 2.25f, -30 * 3.14159 / 180);
	mSq.talk = true;
	Entity bSq(3.0f, 3.0f, 30 * 3.14159 / 180);
	init(sSq, mSq, bSq);

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
			if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					state = GAME_STATE;
				}
			}
			
		}

		glClear(GL_COLOR_BUFFER_BIT);

		program->setProjectionMatrix(projectionMatrix);
		program->setViewMatrix(viewMatrix);

		if (state == GAME_STATE) {
			//fixed updating
			if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
				fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
			}
			while (fixedElapsed >= FIXED_TIMESTEP) {
				fixedElapsed -= FIXED_TIMESTEP;
				update(FIXED_TIMESTEP, sSq, mSq, bSq);
			}
			//update player
			update(fixedElapsed, sSq, mSq, bSq);
			render(sSq, mSq, bSq);
		}
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
