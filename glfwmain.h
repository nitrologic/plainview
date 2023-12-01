#define GLAD_GL_IMPLEMENTATION

#include "glad/gl.h"
#include <GLFW/glfw3.h>

void glfwKeyHandler(GLFWwindow* window, int key, int scancode, int action, int mods) {
	terminateApp = true;
}

struct GLFWDriver :Driver {

	GLFWDriver() {
		int glfwOK = glfwInit();
		if (glfwOK != GLFW_TRUE) {
			std::cout << "glfwInit failure" << std::endl;
			exit(1);
		}
		std::cout << "GLFW " << glfwGetVersionString() << std::endl;
	}
	virtual int addWindow(int w, int h, int hz, int flags) {
		return -1;
	}

	virtual void closeWindow(int frame) {

	}

	virtual void drawQuad(int frame, R dest, RGBA c) {

	}
	
	virtual void clear(int frame) {

	}

	virtual void flip(int frame) {

	}

	void quit() {
		glfwTerminate();
	}

	int test() {
//		int w = 1280;
//		int h = 960;
		int w = 720;
		int h = 576;
		int hz = 75;
		glfwWindowHint(GLFW_REFRESH_RATE, hz);
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		GLFWwindow* window = glfwCreateWindow(w, h, "GLFWDriver test window", monitor, NULL);
//		glfwSetWindowMonitor(window, monitor, 0, 0, w, h, hz);
		glfwMakeContextCurrent(window);
		if (!gladLoadGL(glfwGetProcAddress)) // For GLAD 2 use the following instead: 
		{
			glfwTerminate();
			exit(EXIT_FAILURE);
		}
		glfwSwapInterval(1);
		glfwSetKeyCallback(window, glfwKeyHandler);
		int frameCount = 0;
		while (!terminateApp && !glfwWindowShouldClose(window)){
			int count = (frameCount++);
			float g = 0.2*sin(0.032*count);
			glClearColor(0.2, g, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			glfwSwapBuffers(window);
			glfwPollEvents();
//			glfwWaitEventsTimeout(0.01);
		}
		glfwDestroyWindow(window);
		glfwPollEvents();
		return 0;
	}
};

Driver* glfwOpen() {
	GLFWDriver* driver = new GLFWDriver();
	int monitorCount;
	GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
	GLFWmonitor* primary = glfwGetPrimaryMonitor();
	for (int i = 0; i < monitorCount; i++) {
		GLFWmonitor* monitor = monitors[i];
		const char* name = glfwGetMonitorName(monitor);
		int xpos, ypos;
		int width, height;
		glfwGetMonitorWorkarea(monitor, &xpos, &ypos,&width,&height);
		char pri = (primary == monitor) ? '*' : ' ';
//		std::cout << " monitor #" << i << " " << name << " @ " << xpos << " , " << ypos << " , " << width << " , " << height << " " << pri << std::endl;
		float scale = 1.0;
		addMonitor("GLFW", name, scale, xpos, ypos, width, height);
		int modeCount;
		const GLFWvidmode* mode = glfwGetVideoModes(monitor, &modeCount);
		for (int j = 0; j < modeCount; j++) {
			int w = mode[j].width;
			int h = mode[j].height;
			int hz = mode[j].refreshRate;
			int bits = mode[j].redBits + mode[j].blueBits + mode[j].greenBits;
//			std::cout << "  mode #" << j << " " << w << " x " << h << " " << bits << "bpp " << hz << "hz" << std::endl;
			// assert bits==24
			currentMonitor->addMode(w, h, 1, hz);
		}
	}

	return driver;
}



int glfwMain() {
	std::cout << "plainview " << plainviewVersion << std::endl;
	Driver* glfwDriver = glfwOpen();
	dumpModes();
//	testDriver(glfwDriver);
	//	glfwMain();
	testDriver(glfwDriver);
	glfwDriver->quit();
	return 0;
}
