//#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

#include "OpenGLWindow.h"

int main(int argc, char* argv[])
{

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(windowWidth, windowHeight);
	glutInitContextVersion(4, 1);
	glutInitContextFlags(GLUT_CORE_PROFILE | GLUT_DEBUG);
	glutCreateWindow("John's Window");
	glewInit();

	OpenGLWindow* OpenGL = new OpenGLWindow(argv[1]);

	glutMainLoop();

	delete OpenGL;
}