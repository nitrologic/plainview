/*

plainview.cpp

(c)2023 Simon Armstrong

All Rights Reserved

*/

#include <iostream>

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
	S name;
	Zoom zoom;
	R rect;
	ModeTypes modeTypes;
	Modes modeList;
	Monitor() {
		name = "not a display monitor";
		zoom = 0.0;
	}
	Monitor(S monitorName,float scale,int x, int y,int w,int h) {
		name = monitorName;
		zoom = scale;
		rect = { x,y,w,h };
	}
	Monitor(const Monitor& from) {
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

void addMonitor(S name, F scale, int x, int y, int w, int h) {
	currentMonitor = new Monitor(name,scale,x,y,w,h);
	allMonitors.push_back(currentMonitor);
}

void dumpModes() {
	for (Monitor *m : allMonitors) {
		std::cout << "videodisplay-" << m->name << " ";
		std::cout << m->rect.x << "," << m->rect.y << "," << m->rect.w << "," << m->rect.h;
		std::cout << std::endl;
		for (const DensityFrequency &mode : m->modeTypes) {
			std::cout << "\t" << mode.first << "Zoom " << mode.second << " Hz" << std::endl;
			for (const VideoMode &videoMode : m->modeList[mode]) {
				std::cout << "\t\t" << videoMode.mWidth << " x " << videoMode.mHeight << std::endl;
			}
		}
	}
	std::cout << std::endl;
}

int sdlMain() {
	Uint32 sdlFlags=SDL_INIT_VIDEO;
	if (SDL_Init(sdlFlags) < 0) {
		std::cout << "SDL failure" << std::endl;
		return 1;
	}
	int displayCount;
 	SDL_DisplayID* sdlDisplays = SDL_GetDisplays(&displayCount);
	for(int i=0;i<displayCount;i++){
		SDL_DisplayID displayId = sdlDisplays[i];
		SDL_PropertiesID props = SDL_GetDisplayProperties(displayId);
		SDL_Rect rect;
		const char *name = SDL_GetDisplayName(displayId);
		float scale = SDL_GetDisplayContentScale(displayId);
		int ok = SDL_GetDisplayBounds(displayId, &rect);
		std::cout << "display #" << i << " \"" << name << "\" " << rect.x << "," << rect.y << "," << rect.w<< "," << rect.h << " x" << scale << std::endl;
		addMonitor(name, scale, rect.x, rect.y, rect.w, rect.h);
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
	dumpModes();
	SDL_QuitSubSystem(sdlFlags);
	return 0;
}

int glfwMain() {
	int glfwOK = glfwInit();
	if (glfwOK != GLFW_TRUE) {
		std::cout << "glfwInit failure" << std::endl;
		exit(1);
	}
	std::cout << " GLFW " << glfwGetVersionString() << std::endl;
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
		std::cout << " monitor #" << i << " " << name << " @ " << xpos << " , " << ypos << " , " << width << " , " << height << " " << pri << std::endl;
		float scale = 1.0;
		addMonitor(name, scale, xpos, ypos, width, height);
		int modeCount;
		const GLFWvidmode* mode = glfwGetVideoModes(monitor, &modeCount);
		for (int j = 0; j < modeCount; j++) {
			int w = mode[j].width;
			int h = mode[j].height;
			int hz = mode[j].refreshRate;
			int bits = mode[j].redBits + mode[j].blueBits + mode[j].greenBits;
			std::cout << "  mode #" << j << " " << w << " x " << h << " " << bits << "bpp " << hz << "hz" << std::endl;
			// assert bits==24
			currentMonitor->addMode(w, h, 1, hz);
		}
	}
	dumpModes();
	glfwTerminate();	
    return 0;
}

int main() {
	std::cout << "plainview 0.1" << std::endl;
//	return sdlMain();
	return glfwMain();
}
