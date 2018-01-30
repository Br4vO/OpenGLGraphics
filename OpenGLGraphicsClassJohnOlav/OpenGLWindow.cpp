#include "OpenGLWindow.h"

//Variables
static float sGreen = 0;
static bool sGoingUp = true;

static cyTriMesh* vertexData;
static GLuint g_vertexArrayID;
static GLuint g_vertexBuffer;
static unsigned int nrOfVertices;
static float* matrixPointer;
static cy::GLSLProgram* m_shaderProgram;

static GLuint MatrixID;

static cy::Point2f previousMousePosition;
static float mouseZ;
static float mouseX;
static float mouseY;
static bool leftMouseDown;
static bool rightMouseDown;

OpenGLWindow::OpenGLWindow(const char * filename)
{
	Init(filename);
}

OpenGLWindow::~OpenGLWindow()
{
	delete vertexData;
	delete m_shaderProgram;
	delete[] matrixPointer;
}

void OpenGLWindow::Init(const char * filename)
{

	glutDisplayFunc(Display);
	glutKeyboardFunc(Keyboard);
	glutMouseFunc(Mouse);
	glutMotionFunc(MouseMotion);
	glutSpecialFunc(SpecialInput);
	glutIdleFunc(Idle);

	vertexData = new cyTriMesh();
	m_shaderProgram = new cy::GLSLProgram();

	matrixPointer = new float[16];

	ExtractVertexDataAndGiveToOpenGL(filename);
	LoadAndBuildShaders();
}

bool OpenGLWindow::ExtractVertexDataAndGiveToOpenGL(const char * filename)
{
	if (!vertexData->LoadFromFileObj(filename))
	{
		std::cerr << "Failed to load file.";
		return false;
	}
	unsigned int bufferSize = vertexData->NV() * sizeof(cy::Point3f);
	nrOfVertices = vertexData->NV();

	glGenVertexArrays(1, &g_vertexArrayID);
	glBindVertexArray(g_vertexArrayID);
	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers(1, &g_vertexBuffer);
	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, g_vertexBuffer);
	// Give our vertices to OpenGL.
	glBufferData(GL_ARRAY_BUFFER, bufferSize, &vertexData->V(0), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, g_vertexBuffer);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		sizeof(cy::Point3f),    // stride
		(void*)0            // array buffer offset
	);
	return true;
}

bool OpenGLWindow::LoadAndBuildShaders()
{
	m_shaderProgram->CreateProgram();

	const char * vertexShaderPath = "Shaders\\VertexShader.glsl";
	const char * fragmentShaderPath = "Shaders\\FragmentShader.glsl";

	bool success = m_shaderProgram->BuildFiles(vertexShaderPath, fragmentShaderPath);
	return success;
}

void OpenGLWindow::CalculateMVP()
{
	cy::Matrix4f projectionMatrix;
	projectionMatrix.SetIdentity();
	projectionMatrix.SetPerspective(45.0f, windowWidth / windowHeight, 0.1f, 100.0f);


	cy::Matrix4f viewMatrix;
	viewMatrix.SetIdentity();

	cy::Matrix4f positionMatrix;
	cy::Matrix4f rotationMatrix;
	cy::Matrix4f secondRotationMatrix;

	cy::Point3f position(0, 0, -40 + mouseZ);
	positionMatrix.SetTrans(position);

	cy::Point3f rotationAxis(1, 0, 0);
	rotationMatrix.SetRotation(rotationAxis, -1.5708f + mouseX);

	cy::Point3f secondRotationAxis(0, 1, 0);
	secondRotationMatrix.SetRotation(secondRotationAxis, -1.5708f + mouseY);

	viewMatrix =  positionMatrix * rotationMatrix * secondRotationMatrix;

	cy::Matrix4f modelMatrix;
	modelMatrix.SetIdentity();

	vertexData->ComputeBoundingBox();
	cy::Point3f objectSizes = vertexData->GetBoundMax() - vertexData->GetBoundMin();
	cy::Point3f modelTranslation(0, 0, -(objectSizes.z / 2));
	modelMatrix.SetTrans(modelTranslation);

	cy::Matrix4f MVP = projectionMatrix * viewMatrix * modelMatrix;


	m_shaderProgram->Bind();
	m_shaderProgram->SetUniformMatrix4("MVP", MVP.data);
}

void OpenGLWindow::Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_shaderProgram->Bind();
	CalculateMVP();
	glBindVertexArray(g_vertexArrayID);
	// Draw the triangle !
	glDrawArrays(GL_POINTS, 0, nrOfVertices);

	glutSwapBuffers();
}

void OpenGLWindow::Keyboard(unsigned char key, int x, int y)
{
	if (key == 27)
		exit(0);
}

void OpenGLWindow::Mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON)
	{
		if (state == GLUT_UP)
		{
			leftMouseDown = false;

		}
		else if (state == GLUT_DOWN)
		{
			leftMouseDown = true;
			previousMousePosition.Set(x, y);
		}
	}
	else if (button == GLUT_RIGHT_BUTTON)
	{
		if (state == GLUT_UP)
		{
			rightMouseDown = false;
			
		}
		else if (state == GLUT_DOWN)
		{
			rightMouseDown = true;
			previousMousePosition.Set(x, y);
		}
	}
}

void OpenGLWindow::MouseMotion(int x, int y)
{
	if (leftMouseDown)
	{ 
		const double xDiff = previousMousePosition.x - x;
		const double yDiff = previousMousePosition.y - y;
		mouseX += xDiff * 0.001;
		mouseY += yDiff * 0.001;
	}

	if (rightMouseDown)
	{
		const double yDiff = previousMousePosition.y - y;
		mouseZ += (yDiff * 0.001);
	}

}

void OpenGLWindow::SpecialInput(int key, int x, int y)
{
	if (key == GLUT_KEY_F6)
	{
		LoadAndBuildShaders();
	}
}

void OpenGLWindow::Idle()
{

	glClearColor(0, 0, 0, 1);

	glutPostRedisplay();
}
