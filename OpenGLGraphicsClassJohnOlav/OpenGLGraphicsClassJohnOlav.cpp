//#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

#include "OpenGLWindow.h"

int main(int argc, char* argv[])
{

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(windowWidth, windowHeight);
	glutCreateWindow("John's Window");
	glewInit();

	OpenGLWindow* OpenGL = new OpenGLWindow("OBJFiles\\teapot.obj");

	glutMainLoop();

	delete OpenGL;
}