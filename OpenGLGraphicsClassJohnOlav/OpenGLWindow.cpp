#include "OpenGLWindow.h"

#include "lodepng.h"
#include "lodepng_util.h"

//Variables
static float sGreen = 0;
static bool sGoingUp = true;

static cyTriMesh* vertexData;

static GLuint g_vertexArrayID;
static GLuint g_vertexArrayPlaneID;
static GLuint g_vertexArrayBoxID;

static GLuint g_vertexBuffer;
static GLuint g_normalBuffer;
static GLuint g_textureCoordBuffer;

static GLuint g_planeVertexBuffer;
static GLuint g_planeNormalBuffer;

static GLuint g_boxVertexBuffer;
static GLuint g_boxNormalBuffer;

static GLuint g_textureIDDiffuse;
static GLuint g_textureIDSpec;

static unsigned int nrOfVertices;
static const unsigned int nrOfPlaneVertices = 6;
static  unsigned int nrOfBoxVertices;

static float* matrixPointer;
static cy::GLSLProgram* g_shaderProgram;
static cy::GLSLProgram* g_shaderProgramPlane;
static cy::GLSLProgram* g_shaderProgramBox;
static cy::GLSLProgram* g_shaderProgramShadow;

static cy::GLRenderTexture<GL_TEXTURE_2D>* g_renderTexture;
static cy::GLRenderDepth2D* g_shadowTexture;
static cy::GLTextureCubeMap cubeMap;

static const cy::Point3f ligthRotationAngle(0, 1, 0);
static const cy::Point3f lightPosition(5, 20, 20);


static GLuint MatrixID;

static cy::Point2f previousMousePosition;
static float mouseZTeapot;
static float mouseXTeapot;
static float mouseYTeapot;
static float mouseZPlane;
static float mouseXPlane;
static float mouseYPlane;

static float lightRotation = 2;
static bool ctrlDown;
static bool altDown;

static bool leftMouseDown;
static bool rightMouseDown;

OpenGLWindow::OpenGLWindow(const char * filename)
{
	Init(filename);
}

OpenGLWindow::~OpenGLWindow()
{
	delete vertexData;
	delete g_shaderProgram;
	delete g_renderTexture;
	delete g_shadowTexture;
	delete g_shaderProgramPlane;
	delete g_shaderProgramBox;
	delete g_shaderProgramShadow;
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

	g_renderTexture = new cy::GLRenderTexture<GL_TEXTURE_2D>();
	g_shadowTexture = new cy::GLRenderDepth2D();

	g_renderTexture->Initialize(true, 3, windowWidth, windowHeight);
	g_renderTexture->SetTextureFilteringMode(GL_LINEAR, 0);
	g_renderTexture->SetTextureMaxAnisotropy();
	g_renderTexture->BuildTextureMipmaps();

	g_shadowTexture->Initialize(true, 1024,1024);

	matrixPointer = new float[16];

	GenerateBox();

	ExtractDataAndGiveToOpenGL(filename);

	GeneratePlane();

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
		auto faceNormal = vertexData->FN(i);
		//auto faceTexture = vertexData->FT(i);

		for (unsigned int j = 0; j < 3; j++)
		{
			auto vertexIndex = face.v[j];
			auto vertexNormalIndex = faceNormal.v[j];
			//auto vertexTextureIndex = faceTexture.v[j];

			cy::Point3f vertex = vertexData->V(vertexIndex);
			vertexDataBuffer.push_back(vertex);

			cy::Point3f normal = vertexData->VN(vertexNormalIndex);
			normalDataBuffer.push_back(normal);

			//cy::Point2f textureVertex = cy::Point2f(vertexData->VT(vertexTextureIndex).x, vertexData->VT(vertexTextureIndex).y);
			//vertexTextureDataBuffer.push_back(textureVertex);
		}
	}
	const char * textureFolder = "Teapot/";

	//std::vector<unsigned char> diffuseBuffer;
	//unsigned dw, dh;
	//{
	//	std::string diffuseName(textureFolder);

	//	diffuseName += vertexData->M(0).map_Kd.data;

	//	auto errorNumber = lodepng::decode(diffuseBuffer, dw, dh, diffuseName, LodePNGColorType::LCT_RGB);
	//	const char * errorMessage;
	//	if (errorNumber != 0)
	//		errorMessage = lodepng_error_text(errorNumber);
	//}

	//std::vector<unsigned char> specularBuffer;
	//unsigned sw, sh;
	//{
	//	std::string specularName(textureFolder);

	//	specularName += vertexData->M(0).map_Ks.data;
	//	auto errorNumber = lodepng::decode(specularBuffer, sw, sh, specularName, LodePNGColorType::LCT_RGB);
	//	const char * errorMessage;
	//	if (errorNumber != 0)
	//		errorMessage = lodepng_error_text(errorNumber);
	//}

	glGenVertexArrays(1, &g_vertexArrayID);
	glBindVertexArray(g_vertexArrayID);

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
	// The following commands will talk about our 'vertexbuffer' buffer
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

	//// Generate 1 buffer, put the resulting identifier in vertexbuffer
	//glGenBuffers(1, &g_textureCoordBuffer);
	//// The following commands will talk about our 'vertexbuffer' buffer
	//glBindBuffer(GL_ARRAY_BUFFER, g_textureCoordBuffer);
	////// Give our vertices to OpenGL.
	//glBufferData(GL_ARRAY_BUFFER, vertexTextureBufferSize, vertexTextureDataBuffer.data(), GL_STATIC_DRAW);
	//glEnableVertexAttribArray(2);
	//glVertexAttribPointer(
	//	2,
	//	2,                  // size
	//	GL_FLOAT,           // type
	//	GL_FALSE,           // normalized?
	//	0,    // stride
	//	(void*)0          // array buffer offset
	//);

	//glGenTextures(1, &g_textureIDDiffuse);

	//glBindTexture(GL_TEXTURE_2D, g_textureIDDiffuse);

	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, dw, dh, 0, GL_RGB, GL_UNSIGNED_BYTE, diffuseBuffer.data());
	//glGenerateMipmap(GL_TEXTURE_2D);


	//glGenTextures(1, &g_textureIDSpec);

	//glBindTexture(GL_TEXTURE_2D, g_textureIDSpec);

	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sw, sh, 0, GL_RGB, GL_UNSIGNED_BYTE, specularBuffer.data());
	//glGenerateMipmap(GL_TEXTURE_2D);

	return true;
}

bool OpenGLWindow::GeneratePlane()
{
	const GLfloat quad_vertex_buffer_data[] = {
		250.0f, 0.0f, 250.0f,
		-250.0f, 0.0f, -250.0f,
		-250.0f,  0.0f, 250.0f,
		250.0f,  0.0f, 250.0f,
		250.0f, 0.0f, -250.0f,
		-250.0f,  0.0f, -250.0f,
	};

	glGenVertexArrays(1, &g_vertexArrayPlaneID);
	glBindVertexArray(g_vertexArrayPlaneID);

	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers(1, &g_planeVertexBuffer);
	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, g_planeVertexBuffer);
	// Give our vertices to OpenGL.
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_buffer_data), quad_vertex_buffer_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,                  
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		sizeof(cy::Point3f),    // stride
		(void*)0            // array buffer offset
	);

	std::vector<cy::Point3f> normals;

	for (unsigned int i = 0; i < nrOfPlaneVertices * 3; i+=9)
	{
		cy::Point3f A(quad_vertex_buffer_data[i], quad_vertex_buffer_data[i + 1], quad_vertex_buffer_data[i + 2]);
		cy::Point3f B(quad_vertex_buffer_data[i+3], quad_vertex_buffer_data[i + 4], quad_vertex_buffer_data[i + 5]);
		cy::Point3f C(quad_vertex_buffer_data[i+6], quad_vertex_buffer_data[i + 7], quad_vertex_buffer_data[i + 8]);

		cy::Point3f N = (A - B).Cross(B - C);
		//cy::Point3f N(0.0f, 0.0f, 1.0f);

		if (N.Length() > 0)
		{
			N.Normalize();
			normals.push_back(N);
			normals.push_back(N);
			normals.push_back(N);
		}

	}

	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers(1, &g_planeNormalBuffer);
	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, g_planeNormalBuffer);
	//// Give our vertices to OpenGL.
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(cy::Point3f), normals.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1,
		3,                  // size
		GL_FLOAT,           // type
		GL_TRUE,           // normalized?
		sizeof(cy::Point3f),    // stride
		(void*)0          // array buffer offset
	);

	const GLfloat UVs[] = {
		1.0f, 1.0f, 
		0.0f, 0.0f, 
		1.0f,  0.0f, 
		1.0f,  1.0f,
		0.0f, 1.0f, 
		0.0f,  0.0f, 
	};

	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers(1, &g_textureCoordBuffer);
	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, g_textureCoordBuffer);
	//// Give our vertices to OpenGL.
	glBufferData(GL_ARRAY_BUFFER, sizeof(UVs), UVs, GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2,
		2,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,    // stride
		(void*)0          // array buffer offset
	);



	return true;
}

bool OpenGLWindow::GenerateBox()
{
	cyTriMesh boxVertexData;
	boxVertexData.LoadFromFileObj("cubemap/cube.obj");

	unsigned int nrOfFaces = boxVertexData.NF();
	nrOfBoxVertices = nrOfFaces * 3;
	unsigned int vertexBufferSize = nrOfBoxVertices * sizeof(cy::Point3f);

	std::vector<cy::Point3f> vertexDataBuffer;

	for (unsigned int i = 0; i < nrOfFaces; i++)
	{
		auto face = boxVertexData.F(i);

		for (unsigned int j = 0; j < 3; j++)
		{
			auto vertexIndex = face.v[j];

			cy::Point3f vertex = boxVertexData.V(vertexIndex);
			vertexDataBuffer.push_back(vertex);
		}
	}

	const char * textureFolder = "cubemap/";

	const char * negxTextureName = "cubemap_negx.png";
	const char * negyTextureName = "cubemap_negy.png";
	const char * negzTextureName = "cubemap_negz.png";
	const char * posxTextureName = "cubemap_posx.png";
	const char * posyTextureName = "cubemap_posy.png";
	const char * poszTextureName = "cubemap_posz.png";

	std::vector<unsigned char> negXBuffer;
	unsigned negxWidth, negxHeight;
	{
		std::string negXName(textureFolder);

		negXName += negxTextureName;

		auto errorNumber = lodepng::decode(negXBuffer, negxWidth, negxHeight, negXName, LodePNGColorType::LCT_RGB);
		const char * errorMessage;
		if (errorNumber != 0)
			errorMessage = lodepng_error_text(errorNumber);
	}

	std::vector<unsigned char> negYBuffer;
	unsigned negyWidth, negyHeight;
	{
		std::string negyName(textureFolder);

		negyName += negyTextureName;

		auto errorNumber = lodepng::decode(negYBuffer, negyWidth, negyHeight, negyName, LodePNGColorType::LCT_RGB);
		const char * errorMessage;
		if (errorNumber != 0)
			errorMessage = lodepng_error_text(errorNumber);
	}

	std::vector<unsigned char> negzBuffer;
	unsigned negzWidth, negzHeight;
	{
		std::string negzName(textureFolder);

		negzName += negzTextureName;

		auto errorNumber = lodepng::decode(negzBuffer, negzWidth, negzHeight, negzName, LodePNGColorType::LCT_RGB);
		const char * errorMessage;
		if (errorNumber != 0)
			errorMessage = lodepng_error_text(errorNumber);
	}

	std::vector<unsigned char> posXBuffer;
	unsigned posxWidth, posxHeight;
	{
		std::string posXName(textureFolder);

		posXName += posxTextureName;

		auto errorNumber = lodepng::decode(posXBuffer, posxWidth, posxHeight, posXName, LodePNGColorType::LCT_RGB);
		const char * errorMessage;
		if (errorNumber != 0)
			errorMessage = lodepng_error_text(errorNumber);
	}

	std::vector<unsigned char> posYBuffer;
	unsigned posyWidth, posyHeight;
	{
		std::string posyName(textureFolder);

		posyName += posyTextureName;

		auto errorNumber = lodepng::decode(posYBuffer, posyWidth, posyHeight, posyName, LodePNGColorType::LCT_RGB);
		const char * errorMessage;
		if (errorNumber != 0)
			errorMessage = lodepng_error_text(errorNumber);
	}

	std::vector<unsigned char> poszBuffer;
	unsigned poszWidth, poszHeight;
	{
		std::string poszName(textureFolder);

		poszName += poszTextureName;

		auto errorNumber = lodepng::decode(poszBuffer, poszWidth, poszHeight, poszName, LodePNGColorType::LCT_RGB);
		const char * errorMessage;
		if (errorNumber != 0)
			errorMessage = lodepng_error_text(errorNumber);
	}

	cubeMap.Initialize();
	cubeMap.SetImage(cy::GLTextureCubeMap::NEGATIVE_X, negXBuffer.data(), 3, negxWidth, negxHeight);
	cubeMap.SetImage(cy::GLTextureCubeMap::NEGATIVE_Y, negYBuffer.data(), 3, negyWidth, negyHeight);
	cubeMap.SetImage(cy::GLTextureCubeMap::NEGATIVE_Z, negzBuffer.data(), 3, negzWidth, negzHeight);
	cubeMap.SetImage(cy::GLTextureCubeMap::POSITIVE_X, posXBuffer.data(), 3, posxWidth, posxHeight);
	cubeMap.SetImage(cy::GLTextureCubeMap::POSITIVE_Y, posYBuffer.data(), 3, posyWidth, posyHeight);
	cubeMap.SetImage(cy::GLTextureCubeMap::POSITIVE_Z, poszBuffer.data(), 3, poszWidth, poszHeight);

	cubeMap.BuildMipmaps();

	glGenVertexArrays(1, &g_vertexArrayBoxID);
	glBindVertexArray(g_vertexArrayBoxID);

	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers(1, &g_boxVertexBuffer);
	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, g_boxVertexBuffer);
	// Give our vertices to OpenGL.
	glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, vertexDataBuffer.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,
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
	g_shaderProgram = new cy::GLSLProgram();
	g_shaderProgramPlane = new cy::GLSLProgram();
	g_shaderProgramBox = new cy::GLSLProgram();
	g_shaderProgramShadow = new cy::GLSLProgram();

	g_shaderProgram->CreateProgram();

	const char * vertexShaderPath = "Shaders\\VertexShader.glsl";
	const char * fragmentShaderPath = "Shaders\\FragmentShader.glsl";

	bool success = g_shaderProgram->BuildFiles(vertexShaderPath, fragmentShaderPath);

	g_shaderProgramPlane->CreateProgram();

	vertexShaderPath = "Shaders\\VertexShaderPlane.glsl";
	fragmentShaderPath = "Shaders\\FragmentShaderPlane.glsl";
	success = g_shaderProgramPlane->BuildFiles(vertexShaderPath, fragmentShaderPath);

	g_shaderProgramBox->CreateProgram();

	vertexShaderPath = "Shaders\\VertexShaderBox.glsl";
	fragmentShaderPath = "Shaders\\FragmentShaderBox.glsl";
	success = g_shaderProgramBox->BuildFiles(vertexShaderPath, fragmentShaderPath);

	g_shaderProgramShadow->CreateProgram();

	vertexShaderPath = "Shaders\\VertexShaderShadow.glsl";
	fragmentShaderPath = "Shaders\\FragmentShaderShadow.glsl";
	success = g_shaderProgramShadow->BuildFiles(vertexShaderPath, fragmentShaderPath);

	return success;
}

void OpenGLWindow::CalculateMVPTeapot(bool i_camera)
{
	cy::Matrix4f projectionMatrix;
	projectionMatrix.SetIdentity();
	projectionMatrix.SetPerspective(45.0f, windowWidth / windowHeight, 0.1f, 1000.0f);


	cy::Matrix4f viewMatrix;
	viewMatrix.SetIdentity();

	cy::Matrix4f positionMatrix;
	cy::Matrix4f rotationMatrix;
	cy::Matrix4f secondRotationMatrix;

	cy::Point3f position(0, 0, -40 + mouseZTeapot);
	positionMatrix.SetTrans(position);

	cy::Point3f rotationAxis(1, 0, 0);
	rotationMatrix.SetRotation(rotationAxis, mouseYTeapot);

	cy::Point3f secondRotationAxis(0, 1, 0);
	secondRotationMatrix.SetRotation(secondRotationAxis, mouseXTeapot);

	viewMatrix =  positionMatrix * rotationMatrix * secondRotationMatrix;

	cy::Matrix4f lightRotationMatrix;
	cy::Matrix4f lightTranslationMatrix;
	lightTranslationMatrix.SetIdentity();
	lightRotationMatrix.SetIdentity();

	lightRotationMatrix.SetRotation(ligthRotationAngle, lightRotation);
	lightTranslationMatrix.AddTrans(lightPosition);

	cy::Matrix4f lightMatrix = lightRotationMatrix * lightTranslationMatrix;
	cy::Point3f lPosition = lightMatrix.GetTrans();

	cy::Matrix4f modelMatrix;
	modelMatrix.SetIdentity();

	vertexData->ComputeBoundingBox();
	cy::Point3f objectSizes = vertexData->GetBoundMax() - vertexData->GetBoundMin();

	cy::Point3f modelTranslation(0, 0, 0/*-(objectSizes.z / 2)*/);
	if (i_camera)
	{
		modelTranslation = lPosition;
	}

	modelMatrix.SetTrans(modelTranslation);

	cy::Matrix3f MV = viewMatrix.GetSubMatrix3() * modelMatrix.GetSubMatrix3();

	MV.Invert();
	MV.Transpose();

	cy::Matrix4f MVP = projectionMatrix * viewMatrix * modelMatrix;


	g_shaderProgram->Bind();
	g_shaderProgram->SetUniformMatrix4("MVP", MVP.data);
	g_shaderProgram->SetUniformMatrix3("MV", MV.data);

	g_shaderProgram->SetUniformMatrix4("ModelMat", modelMatrix.data);
	g_shaderProgram->SetUniformMatrix4("ViewMat", viewMatrix.data);
	g_shaderProgram->SetUniformMatrix4("ProjMat", projectionMatrix.data);


	g_shaderProgram->SetUniform3("lightPosition", 1, lPosition.Data());
	g_shaderProgram->SetUniform3("cameraPosition", 1, position.Data());

	cy::Point3f lightAmbientInt(2, 2, 2);
	cy::Point3f lightDiffuseInt(2, 2, 2);
	cy::Point3f lightSpecularInt(2, 2,2);

	g_shaderProgram->SetUniform3("lightAmbientIntensity", 1, lightAmbientInt.Data());
	g_shaderProgram->SetUniform3("lightDiffuseIntensity", 1, lightDiffuseInt.Data());
	g_shaderProgram->SetUniform3("lightSpecularIntensity", 1, lightSpecularInt.Data());

	cy::Point3f matAmbientReflect(0.5, 0.5, 0.5 );
	cy::Point3f matDiffuseReflect(0.5, 0.5, 0.5 );
	cy::Point3f matSpecularReflect(0.5, 0.5, 0.5);

	g_shaderProgram->SetUniform3("matAmbientReflectance", 1, matAmbientReflect.Data());
	g_shaderProgram->SetUniform3("matDiffuseReflectance", 1, matDiffuseReflect.Data());
	g_shaderProgram->SetUniform3("matSpecularReflectance", 1, matSpecularReflect.Data());

	g_shaderProgram->SetUniform("matShininess", 32.0f);
}

void OpenGLWindow::CalculateMVPPlane()
{
	cy::Matrix4f projectionMatrix;
	projectionMatrix.SetIdentity();
	projectionMatrix.SetPerspective(45.0f, windowWidth / windowHeight, 0.1f, 1000.0f);


	cy::Matrix4f viewMatrix;
	viewMatrix.SetIdentity();

	cy::Matrix4f positionMatrix;
	cy::Matrix4f rotationMatrix;
	cy::Matrix4f secondRotationMatrix;

	cy::Point3f position(0, 0, -40 + mouseZTeapot);
	positionMatrix.SetTrans(position);

	cy::Point3f rotationAxis(1, 0, 0);
	rotationMatrix.SetRotation(rotationAxis, mouseYTeapot);

	cy::Point3f secondRotationAxis(0, 1, 0);
	secondRotationMatrix.SetRotation(secondRotationAxis, mouseXTeapot);

	viewMatrix = positionMatrix * rotationMatrix * secondRotationMatrix;

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


	g_shaderProgramPlane->Bind();
	g_shaderProgramPlane->SetUniformMatrix4("MVP", MVP.data);
	g_shaderProgramPlane->SetUniformMatrix3("MV", MV.data);

	g_shaderProgramPlane->SetUniformMatrix4("ModelMat", modelMatrix.data);
	g_shaderProgramPlane->SetUniformMatrix4("ViewMat", viewMatrix.data);
	g_shaderProgramPlane->SetUniformMatrix4("ProjMat", projectionMatrix.data);

	cy::Matrix4f lightRotationMatrix;
	cy::Matrix4f lightTranslationMatrix;
	lightTranslationMatrix.SetIdentity();
	lightRotationMatrix.SetIdentity();

	lightRotationMatrix.SetRotation(ligthRotationAngle, lightRotation);
	lightTranslationMatrix.AddTrans(lightPosition);

	cy::Matrix4f lightMatrix = lightRotationMatrix * lightTranslationMatrix;
	cy::Point3f lPosition = lightMatrix.GetTrans();

	lightMatrix.SetView(lPosition, cy::Point3f(0, 0, -10), cy::Point3f(0, 1, 0));

	g_shaderProgramPlane->SetUniform3("lightPosition", 1, lPosition.Data());
	g_shaderProgramPlane->SetUniform3("cameraPosition", 1, position.Data());

	cy::Point3f lightAmbientInt(1, 1, 1);
	cy::Point3f lightDiffuseInt(1, 1, 1);
	cy::Point3f lightSpecularInt(1, 1, 1);

	g_shaderProgramPlane->SetUniform3("lightAmbientIntensity", 1, lightAmbientInt.Data());
	g_shaderProgramPlane->SetUniform3("lightDiffuseIntensity", 1, lightDiffuseInt.Data());
	g_shaderProgramPlane->SetUniform3("lightSpecularIntensity", 1, lightSpecularInt.Data());

	cy::Point3f matAmbientReflect(0.5, 0.5, 0.5);
	cy::Point3f matDiffuseReflect(0.5, 0.5, 0.5);
	cy::Point3f matSpecularReflect(0.5, 0.5, 0.5);

	g_shaderProgramPlane->SetUniform3("matAmbientReflectance", 1, matAmbientReflect.Data());
	g_shaderProgramPlane->SetUniform3("matDiffuseReflectance", 1, matDiffuseReflect.Data());
	g_shaderProgramPlane->SetUniform3("matSpecularReflectance", 1, matSpecularReflect.Data());

	g_shaderProgramPlane->SetUniform("matShininess", 64.0f);
}

void OpenGLWindow::CalculateMVPBox()
{
	cy::Matrix4f projectionMatrix;
	projectionMatrix.SetIdentity();
	projectionMatrix.SetPerspective(45.0f, windowWidth / windowHeight, 0.1f, 100.0f);

	cy::Matrix4f viewMatrix;
	viewMatrix.SetIdentity();

	cy::Matrix4f positionMatrix;
	cy::Matrix4f rotationMatrix;
	cy::Matrix4f secondRotationMatrix;

	cy::Point3f rotationAxis(1, 0, 0);
	rotationMatrix.SetRotation(rotationAxis, mouseYTeapot);

	cy::Point3f secondRotationAxis(0, 1, 0);
	secondRotationMatrix.SetRotation(secondRotationAxis, mouseXTeapot);

	viewMatrix = rotationMatrix * secondRotationMatrix;

	g_shaderProgramBox->Bind();

	g_shaderProgramBox->SetUniformMatrix4("V", viewMatrix.data);
	g_shaderProgramBox->SetUniformMatrix4("P", projectionMatrix.data);
}

void OpenGLWindow::CalculateMVPShadow()
{
	cy::Matrix4f projectionMatrix;
	projectionMatrix.SetIdentity();
	projectionMatrix.SetPerspective(45.0f, 1, 2.0f, 100.0f);

	cy::Matrix4f lightRotationMatrix;
	cy::Matrix4f lightTranslationMatrix;
	lightTranslationMatrix.SetIdentity();
	lightRotationMatrix.SetIdentity();

	lightRotationMatrix.SetRotation(ligthRotationAngle, lightRotation);
	lightTranslationMatrix.AddTrans(lightPosition);

	cy::Matrix4f lightMatrix = lightRotationMatrix * lightTranslationMatrix;
	cy::Point3f lPosition = lightMatrix.GetTrans();

	cy::Matrix4f viewMatrix;
	viewMatrix.SetIdentity();

	//viewMatrix = lightMatrix;
	viewMatrix.SetView(lPosition, cy::Point3f(0,0,0), cy::Point3f(0, 1, 0));
	

	cy::Matrix4f modelMatrix;
	modelMatrix.SetIdentity();

	cy::Matrix4f MVP = projectionMatrix * viewMatrix * modelMatrix;


	g_shaderProgramShadow->Bind();
	g_shaderProgramShadow->SetUniformMatrix4("MVP", MVP.data);

	//g_shaderProgramShadow->SetUniformMatrix4("ModelMat", modelMatrix.data);
	//g_shaderProgramShadow->SetUniformMatrix4("ViewMat", viewMatrix.data);
	//g_shaderProgramShadow->SetUniformMatrix4("ProjMat", projectionMatrix.data);


	//g_shaderProgramShadow->SetUniform3("lightPosition", 1, lightPosition.Data());

	cy::Matrix4f biasMatrix(
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 0.5, 0.0,
		0.5, 0.5, 0.5, 1.0
	);

	cy::Matrix4f depthBiasMVP = /*biasMatrix */ projectionMatrix * viewMatrix;
	g_shaderProgram->Bind();
	g_shaderProgram->SetUniformMatrix4("DepthBiasMVP", depthBiasMVP.data);

	g_shaderProgramPlane->Bind();
	g_shaderProgramPlane->SetUniformMatrix4("DepthBiasMVP", depthBiasMVP.data);
	
	g_shaderProgramShadow->Bind();
}

void OpenGLWindow::Display()
{

	//{

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0, 0, 0, 1);

	//	glDepthMask(GL_FALSE);
	//	g_shaderProgramBox->Bind();
	//	CalculateMVPBox();
	//	glBindVertexArray(g_vertexArrayBoxID);

	//	glActiveTexture(GL_TEXTURE0);
	//	cubeMap.Bind();

	//	glDrawArrays(GL_TRIANGLES, 0, nrOfBoxVertices);
	//	glDepthMask(GL_TRUE);
	//}


	{
		g_shadowTexture->Bind();
		glClearDepth(1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0, 0, 0, 1);

		g_shaderProgramShadow->Bind();
		CalculateMVPShadow();
		glBindVertexArray(g_vertexArrayID);

		glDrawArrays(GL_TRIANGLES, 0, nrOfVertices);

		g_shadowTexture->Unbind();
	}

	{
		g_shaderProgram->Bind();
		CalculateMVPTeapot(false);
		glBindVertexArray(g_vertexArrayID);


		glActiveTexture(GL_TEXTURE0);
		g_shadowTexture->BindTexture();

		glDrawArrays(GL_TRIANGLES, 0, nrOfVertices);
	}

	{
		g_shaderProgram->Bind();
		CalculateMVPTeapot(true);
		glBindVertexArray(g_vertexArrayID);

		glActiveTexture(GL_TEXTURE0);
		g_shadowTexture->BindTexture();

		glDrawArrays(GL_TRIANGLES, 0, nrOfVertices);
	}

	{
		g_shaderProgramPlane->Bind();
		CalculateMVPPlane();
		glBindVertexArray(g_vertexArrayPlaneID);

		glActiveTexture(GL_TEXTURE0);
		g_shadowTexture->BindTexture();

		glDrawArrays(GL_TRIANGLES, 0, nrOfPlaneVertices);
	}


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
		if (altDown)
		{
			mouseXPlane += xDiff * 0.001f;
			mouseYPlane += yDiff * 0.001f;
		}
		else
		{
			mouseXTeapot += xDiff * 0.001f;
			mouseYTeapot += yDiff * 0.001f;
		}


	}

	else if (rightMouseDown)
	{
		const float yDiff = previousMousePosition.y - y;
		if (altDown)
			mouseZPlane += (yDiff * 0.001f);
		else
			mouseZTeapot += (yDiff * 0.001f);
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
	else if (key == GLUT_KEY_ALT_L)
	{
		altDown = true;
	}
}

void OpenGLWindow::SpecialInputUp(int key, int x, int y)
{
	if (key == GLUT_KEY_CTRL_L)
	{
		ctrlDown = false;
	}
	else if (key == GLUT_KEY_ALT_L)
	{
		altDown = false;
	}
}

void OpenGLWindow::Idle()
{

	glutPostRedisplay();
}
