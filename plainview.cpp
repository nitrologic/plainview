/*

plainview.cpp

Copyright © 2023 Simon Armstrong

All Rights Reserved

*/

const char *plainviewVersion = "0.2";

static bool terminateApp = false;

#include <iostream>

#define GLAD_GL_IMPLEMENTATION

#include "glad/gl.h"
#include <GLFW/glfw3.h>

#include <SDL3/SDL.h>

#include <map>;
#include <vector>;
#include <set>;

struct Rectangle {
	int x,y,w,h;
	Rectangle() {
		x = y = w = h = 0;
	}
	Rectangle(int left,int top,int width,int height) {
		x = left;
		y = top;
		w = width;
		h = height;
	}
};

struct VideoMode {
	int mWidth, mHeight;
	VideoMode(VideoMode& other) {
		mWidth = other.mWidth;
		mHeight = other.mHeight;
	}
	VideoMode(int width, int height) {
		mWidth = width;
		mHeight = height;
	}
};

typedef double Hz;
typedef double Zoom;

typedef std::pair<Zoom, Hz> DensityFrequency;
typedef std::set<DensityFrequency> ModeTypes;
typedef std::vector<VideoMode> ModeList;
typedef std::map< DensityFrequency, ModeList> Modes;

typedef std::string S;
typedef float F;

typedef Rectangle R;

struct Monitor {
	S driver;
	S name;
	Zoom zoom;
	R rect;
	ModeTypes modeTypes;
	Modes modeList;
	Monitor() {
		name = "not a display monitor";
		zoom = 0.0;
	}
	Monitor(S api,S monitorName,float scale,int x, int y,int w,int h) {
		driver = api;
		name = monitorName;
		zoom = scale;
		rect = { x,y,w,h };
	}
	Monitor(const Monitor& from) {
		driver = from.driver;
		name = from.name;
		zoom = from.zoom;

	}
	void addMode(int w, int h, Zoom mag, Hz freq) {
		DensityFrequency df(mag,freq);
		if (modeTypes.count(df)==0) {
			modeTypes.insert(df);
		}
		modeList[df].push_back({w,h});
	}
};

typedef std::vector<Monitor*> Monitors;

Monitors allMonitors;
Monitor NullMonitor;

Monitor *currentMonitor=&NullMonitor;

void addMonitor(S driver,S name, F scale, int x, int y, int w, int h) {
	currentMonitor = new Monitor(driver,name,scale,x,y,w,h);
	allMonitors.push_back(currentMonitor);
}

void dumpModes() {
	for (Monitor *m : allMonitors) {
		std::cout << m->driver << " " << m->name << " ";
		std::cout << m->rect.x << "," << m->rect.y << "," << m->rect.w << "," << m->rect.h;
		std::cout << std::endl;
		for (const DensityFrequency &mode : m->modeTypes) {
			std::cout << "\t" << mode.first << "x " << mode.second << " Hz" << std::endl;
			for (const VideoMode &videoMode : m->modeList[mode]) {
				std::cout << "\t\t" << videoMode.mWidth << " x " << videoMode.mHeight << std::endl;
			}
		}
	}
	std::cout << std::endl;
}

struct Driver {
	int frameCount=0;
	virtual int test() = 0;
	virtual int addWindow(int w, int h, int hz, int flags) = 0;
	virtual void closeWindow(int frame) = 0;
	virtual void flip(int frame) = 0;
	virtual void close() = 0;
};

struct SDLDriver : Driver {

	Uint32 sdlFlags;

	SDLDriver() {
		sdlFlags = SDL_INIT_VIDEO;
		if (SDL_Init(sdlFlags) < 0) {
			std::cout << "SDL failure" << std::endl;
			return;
		}
		SDL_version version;
		SDL_GetVersion(&version);
		std::cout << "SDL " << (int)version.major << "." << (int)version.minor << "." << (int)version.patch << std::endl;
	}

	void close() {
		SDL_QuitSubSystem(sdlFlags);
	}

	std::vector<SDL_Window*> windows;
	std::vector<SDL_Surface*> surfaces;

	void closeWindow(int frame) {
		SDL_Window* window = windows[frame];
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	int addWindow(int w,int h,int hz,int flags){
		SDL_Window* window = NULL;
		window = SDL_CreateWindow("plainview", w, h, flags);
		if (window == NULL) {
			const char *error=SDL_GetError();
			std::cout << "could not create window " << error << std::endl;
			return -1;
		}
		int result = (int)windows.size();
		windows.push_back(window);
		SDL_Surface* surface = SDL_GetWindowSurface(window);
		surfaces.push_back(surface);
		return result;
	}

	void flip(int frame){
		SDL_Surface *surface=surfaces[frame];

		int g = frameCount++ % 100;

		Uint32 c = SDL_MapRGB(surface->format, 44, g, 77);

		SDL_FillSurfaceRect(surface, NULL, c);

		SDL_Window *window=windows[frame];
		SDL_UpdateWindowSurface(window);
	}

// https://discourse.libsdl.org/t/vsync-while-software-rendering-my-solution/26824

	int test() {
		Uint32 flags = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL;	// | SDL_WINDOW_HIGH_PIXEL_DENSITY;
		int wh=addWindow(1280,960,75,flags);
		if(wh<0){
			std::cout << "addWindow failure" << std::endl;
			return -1;
		}
		bool running = true;
		while (running) {
			int count = (frameCount++) % 100;
			int g = count*2;
			flip(wh);
			SDL_Event event;
			if (SDL_WaitEventTimeout(&event, 5)) {
				switch (event.type) {
				case SDL_EVENT_KEY_DOWN:
					running = false;
					break;
				}
			}
			SDL_Delay(5);
//			std::cout << "." << std::endl;
		}
		closeWindow(wh);
	}

	int test2() {
		SDL_Window* window = NULL;
		Uint32 flags = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL;	// | SDL_WINDOW_HIGH_PIXEL_DENSITY;

		int w = 1280;
		int h = 960;
		int hz = 75;

		window = SDL_CreateWindow("plainview", w, h, flags);
		//		"hello_sdl2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,1024,768,SDL_WINDOW_SHOWN);
		if (window == NULL) {
			fprintf(stderr, "could not create window: %s\n", SDL_GetError());
			return 1;
		}

		SDL_Surface* screenSurface = NULL;
		screenSurface = SDL_GetWindowSurface(window);
		int frameCount = 0;

		bool running = true;
		while (running) {//!glfwWindowShouldClose(window)) {
			int count = (frameCount++) % 100;
			int g = count*2;
			SDL_FillSurfaceRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 44, g, 77));
			SDL_UpdateWindowSurface(window);
			SDL_Event event;
			if (SDL_WaitEventTimeout(&event, 5)) {
				switch (event.type) {
				case SDL_EVENT_KEY_DOWN:
					running = false;
					break;
				}
			}
			SDL_Delay(5);
//			std::cout << "." << std::endl;
		}
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

};


Driver *sdlOpen() {
	SDLDriver *driver = new SDLDriver();

	int displayCount;
 	SDL_DisplayID* sdlDisplays = SDL_GetDisplays(&displayCount);
	for(int i=0;i<displayCount;i++){
		SDL_DisplayID displayId = sdlDisplays[i];
		SDL_PropertiesID props = SDL_GetDisplayProperties(displayId);
		SDL_Rect rect;
		const char *name = SDL_GetDisplayName(displayId);
		float scale = SDL_GetDisplayContentScale(displayId);
		int ok = SDL_GetDisplayBounds(displayId, &rect);
//		std::cout << "display #" << i << " \"" << name << "\" " << rect.x << "," << rect.y << "," << rect.w<< "," << rect.h << " x" << scale << std::endl;
		addMonitor("SDL", name, scale, rect.x, rect.y, rect.w, rect.h);
		int modeCount;
		const SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(displayId, &modeCount);
		while (const SDL_DisplayMode *mode = *modes++) {
//			std::cout << mode->w << "," << mode->h << " @" << mode->refresh_rate << "hz ";
//			std::cout << " x" << mode->pixel_density;
//			std::cout << std::endl;
			currentMonitor->addMode(mode->w, mode->h, mode->pixel_density, mode->refresh_rate);
		}
		displayId++;
	}
	return driver;
}

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

	virtual void flip(int frame) {

	}

	void close() {
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

int testDriver(Driver* driver) {
	return driver->test();
}

int glfwMain() {
	std::cout << "plainview " << plainviewVersion << std::endl;
	Driver* glfwDriver = glfwOpen();
	dumpModes();
//	testDriver(glfwDriver);
	//	glfwMain();
	
	testDriver(glfwDriver);

	//sdlDriver->close();
	glfwDriver->close();
	return 0;
}


int sdlMain() {
	std::cout << "plainview " << plainviewVersion << std::endl;

	Driver *sdlDriver = sdlOpen();
	dumpModes();
	testDriver(sdlDriver);
	sdlDriver->close();
	return 0;
}


int main() {
	return sdlMain();
}