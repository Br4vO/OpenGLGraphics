#pragma once
#include "glew.h"
#include "freeglut.h"

#include "cyTriMesh.h"
#include "cyGL.h"
#include "cyMatrix.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

//Window size
static const int windowWidth = 800;
static const int windowHeight = 600;

class OpenGLWindow
{
public:
	OpenGLWindow(const char * filename);
	~OpenGLWindow();

private:
	//Functions
	void Init(const char * filename);
	bool ExtractVertexDataAndGiveToOpenGL(const char * filename);

	static bool LoadAndBuildShaders();
	static void CalculateMVP();


	//Event Functions
	static void Display();
	static void Keyboard(unsigned char key, int x, int y);
	static void Mouse(int button, int state, int x, int y);
	static void MouseMotion(int x, int y);
	static void SpecialInput(int key, int x, int y);
	static void SpecialInputUp(int key, int x, int y);
	static void Idle();

	//Variables




};

