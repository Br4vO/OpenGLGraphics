#include "OpenGLWindow.h"

#include "lodepng.h"
#include "lodepng_util.h"

//Variables
static float sGreen = 0;
static bool sGoingUp = true;

static cyTriMesh* vertexData;
static GLuint g_vertexArrayID;
static GLuint g_vertexBuffer;
static GLuint g_normalBuffer;
static GLuint g_textureCoordBuffer;

static unsigned int nrOfVertices;
static float* matrixPointer;
static cy::GLSLProgram* m_shaderProgram;
static cy::GLTexture2<GL_TEXTURE2>* g_diffuseTexture;
static cy::GLTexture2<GL_TEXTURE2>* g_specularTexture;

static GLuint MatrixID;

static cy::Point2f previousMousePosition;
static float mouseZ;
static float mouseX;
static float mouseY;

static float lightRotation;
static bool ctrlDown;

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
	glutSpecialUpFunc(SpecialInputUp);
	glutIdleFunc(Idle);

	glEnable(GL_DEPTH_TEST);

	vertexData = new cyTriMesh();
	m_shaderProgram = new cy::GLSLProgram();

	matrixPointer = new float[16];

	ExtractDataAndGiveToOpenGL(filename);
	LoadAndBuildShaders();
}

bool OpenGLWindow::ExtractDataAndGiveToOpenGL(const char * filename)
{

	if (!vertexData->LoadFromFileObj(filename, true))
	{
		std::cerr << "Failed to load file.";
		return false;
	}
	
	unsigned int nrOfFaces = vertexData->NF();
	nrOfVertices = nrOfFaces *3;
	unsigned int vertexBufferSize = nrOfVertices * sizeof(cy::Point3f);

	std::vector<cy::Point3f> vertexDataBuffer;

	std::vector<cy::Point3f> normalDataBuffer;

	std::vector<cy::Point2f> vertexTextureDataBuffer;

	unsigned int normalBufferSize = vertexBufferSize;
	unsigned int vertexTextureBufferSize = nrOfVertices * sizeof(cy::Point2f);
	vertexData->ComputeNormals(false);

	for (unsigned int i = 0; i < nrOfFaces; i++)
	{
		auto face = vertexData->F(i);

		for (unsigned int j = 0; j < 3; j++)
		{
			auto vertexIndex = face.v[j];
			cy::Point3f vertex = vertexData->V(vertexIndex);
			vertexDataBuffer.push_back(vertex);

			cy::Point3f normal = vertexData->VN(vertexIndex);
			normalDataBuffer.push_back(normal);

			cy::Point2f textureVertex = cy::Point2f(vertexData->VT(vertexIndex).x, vertexData->VT(vertexIndex).y) ;
			vertexTextureDataBuffer.push_back(textureVertex);
		}
	}
	const char * textureFolder = "teapot/";

	std::vector<unsigned char> diffuseBuffer;
	unsigned dw, dh;
	{
		std::string diffuseName(textureFolder);

		diffuseName += vertexData->M(0).map_Kd.data;
		auto errorNumber = lodepng::decode(diffuseBuffer, dw, dh, diffuseName);
		const char * errorMessage;
		if (errorNumber != 0)
			errorMessage = lodepng_error_text(errorNumber);
	}

	std::vector<unsigned char> specularBuffer;
	unsigned sw, sd;
	{
		std::string specularName(textureFolder);

		specularName += vertexData->M(0).map_Ks.data;
		auto errorNumber = lodepng::decode(specularBuffer, sw, sd, specularName);
		const char * errorMessage;
		if (errorNumber != 0)
			errorMessage = lodepng_error_text(errorNumber);
	}

	glGenVertexArrays(1, &g_vertexArrayID);
	glBindVertexArray(g_vertexArrayID);

	g_diffuseTexture = new cy::GLTexture2<GL_TEXTURE2>();
	g_diffuseTexture->Initialize();
	g_diffuseTexture->SetImageRGBA(diffuseBuffer.data(), dw,dh, 8);
	g_diffuseTexture->BuildMipmaps();
	g_diffuseTexture->SetMaxAnisotropy();


	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers(1, &g_vertexBuffer);
	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, g_vertexBuffer);
	// Give our vertices to OpenGL.
	glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, vertexDataBuffer.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		sizeof(cy::Point3f),    // stride
		(void*)0            // array buffer offset
	);

	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers(1, &g_normalBuffer);
	// The following commands will talk about our 'verteø-æ.5xbuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, g_normalBuffer);
	//// Give our vertices to OpenGL.
	glBufferData(GL_ARRAY_BUFFER, normalBufferSize, normalDataBuffer.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1,                  
		3,                  // size
		GL_FLOAT,           // type
		GL_TRUE,           // normalized?
		sizeof(cy::Point3f),    // stride
		(void*)0          // array buffer offset
	);

	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers(1, &g_textureCoordBuffer);
	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, g_textureCoordBuffer);
	//// Give our vertices to OpenGL.
	glBufferData(GL_ARRAY_BUFFER, vertexTextureBufferSize, vertexTextureDataBuffer.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2,
		2,                  // size
		GL_FLOAT,           // type
		GL_TRUE,           // normalized?
		sizeof(cy::Point2f),    // stride
		(void*)0          // array buffer offset
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

	cy::Matrix3f MV = viewMatrix.GetSubMatrix3() * modelMatrix.GetSubMatrix3();

	MV.Invert();
	MV.Transpose();

	cy::Matrix4f MVP = projectionMatrix * viewMatrix * modelMatrix;


	m_shaderProgram->Bind();
	m_shaderProgram->SetUniformMatrix4("MVP", MVP.data);
	m_shaderProgram->SetUniformMatrix3("MV", MV.data);

	m_shaderProgram->SetUniformMatrix4("ModelMat", modelMatrix.data);
	m_shaderProgram->SetUniformMatrix4("ViewMat", viewMatrix.data);
	m_shaderProgram->SetUniformMatrix4("ProjMat", projectionMatrix.data);

	cy::Matrix4f lightRotationMatrix;
	cy::Matrix4f lightTranslationMatrix;
	lightRotationMatrix.SetIdentity();
	cy::Point3f ligthRotationAngle(0, 0, 1);
	cy::Point3f lightPosition(0, 0, -100);

	lightRotationMatrix.SetRotation(ligthRotationAngle, lightRotation);
	lightTranslationMatrix.AddTrans(lightPosition);

	cy::Matrix4f lightMatrix = lightRotationMatrix * lightTranslationMatrix;
	lightPosition = lightMatrix.GetTrans();


	m_shaderProgram->SetUniform3("lightPosition", 1, lightPosition.Data());
	m_shaderProgram->SetUniform3("cameraPosition", 1, position.Data());

	cy::Point3f lightAmbientInt(1, 1, 1);
	cy::Point3f lightDiffuseInt(1, 1, 1);
	cy::Point3f lightSpecularInt(1, 1, 1);

	m_shaderProgram->SetUniform3("lightAmbientIntensity", 1, lightAmbientInt.Data());
	m_shaderProgram->SetUniform3("lightDiffuseIntensity", 1, lightDiffuseInt.Data());
	m_shaderProgram->SetUniform3("lightSpecularIntensity", 1, lightSpecularInt.Data());

	cy::Point3f matAmbientReflect(0.5, 0.5, 0.5 );
	cy::Point3f matDiffuseReflect(0.5, 0.5, 0.5 );
	cy::Point3f matSpecularReflect(0.5, 0.5, 0.5);

	m_shaderProgram->SetUniform3("matAmbientReflectance", 1, matAmbientReflect.Data());
	m_shaderProgram->SetUniform3("matDiffuseReflectance", 1, matDiffuseReflect.Data());
	m_shaderProgram->SetUniform3("matSpecularReflectance", 1, matSpecularReflect.Data());

	m_shaderProgram->SetUniform("matShininess", 64.0f);

	g_diffuseTexture->Bind();
	m_shaderProgram->SetUniform("texUnit", g_diffuseTexture->GetID());
}

void OpenGLWindow::Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_shaderProgram->Bind();
	CalculateMVP();
	glBindVertexArray(g_vertexArrayID);
	glDrawArrays(GL_TRIANGLES, 0, nrOfVertices);

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
			previousMousePosition.Set(static_cast<float>(x), static_cast<float>(y));
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
			previousMousePosition.Set(static_cast<float>(x), static_cast<float>(y));
		}
	}
}

void OpenGLWindow::MouseMotion(int x, int y)
{
	if (ctrlDown && leftMouseDown)
	{
		const float yDiff = previousMousePosition.y - y;
		lightRotation += (yDiff * 0.001f);
	}
	else if (leftMouseDown)
	{ 
		const float xDiff = previousMousePosition.x - x;
		const float yDiff = previousMousePosition.y - y;
		mouseX += xDiff * 0.001f;
		mouseY += yDiff * 0.001f;
	}

	else if (rightMouseDown)
	{
		const float yDiff = previousMousePosition.y - y;
		mouseZ += (yDiff * 0.001f);
	}

}

void OpenGLWindow::SpecialInput(int key, int x, int y)
{
	if (key == GLUT_KEY_F6)
	{
		LoadAndBuildShaders();
	}
	else if (key == GLUT_KEY_CTRL_L)
	{
		ctrlDown = true;
	}
}

void OpenGLWindow::SpecialInputUp(int key, int x, int y)
{
	if (key == GLUT_KEY_CTRL_L)
	{
		ctrlDown = false;
	}
}

void OpenGLWindow::Idle()
{

	glClearColor(0, 0, 0, 1);

	glutPostRedisplay();
}
