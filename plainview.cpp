/*

plainview.cpp

Copyright © 2023 Simon Armstrong

All Rights Reserved

*/

const char *plainviewVersion = "0.2";

static bool terminateApp = false;

#include <iostream>
#include <map>
#include <vector>
#include <set>

#include <SDL3/SDL.h>

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

typedef uint32_t RGBA;
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

	virtual void clear(int frame) = 0;
	virtual void drawQuad(int frame, R dest,RGBA c) = 0;
	virtual void flip(int frame) = 0;
	virtual void quit() = 0;
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

	void quit() {
		SDL_QuitSubSystem(sdlFlags);
	}

	struct SDLFrame {
		SDL_Window* window;
		SDL_Surface* surface;
		SDL_Renderer* renderer;
	};

	std::vector<SDLFrame> frames;

	void closeWindow(int frame) {
		SDLFrame& sdlFrame = frames[frame];
		SDL_Window* window = sdlFrame.window;
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	int addWindow(int w, int h, int hz, int flags) {
		SDL_Window* window = NULL;
		window = SDL_CreateWindow("plainview", w, h, flags);
		if (window == NULL) {
			const char* error = SDL_GetError();
			std::cout << "could not create window " << error << std::endl;
			return -1;
		}
		SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		SDL_Surface* surface = SDL_GetWindowSurface(window);
		int result = (int)frames.size();
		frames.push_back({ window,surface,renderer });
		return result;
	}

	void clear(int frame) {
		SDLFrame& sdlFrame = frames[frame];
		SDL_Surface* surface = sdlFrame.surface;
		uint32_t c = 0;
		SDL_FillSurfaceRect(surface, NULL, c);
	}

	void drawQuad(int frame, R dest, RGBA c) {
		SDLFrame& sdlFrame = frames[frame];
		SDL_Surface* surface = sdlFrame.surface;
		const SDL_Rect rect = { dest.x,dest.y,dest.w,dest.h };
		SDL_FillSurfaceRect(surface, &rect, c);
	}

	void flip(int frame) {
		SDLFrame& sdlFrame = frames[frame];
		SDL_Window* window = sdlFrame.window;
		Uint32 c = 0xffff00ff;
		int x = 10 + frameCount % 40;
		drawQuad(frame, { x * 10,10,40,40 }, c);
		SDL_UpdateWindowSurface(window);
	}

	// https://discourse.libsdl.org/t/vsync-while-software-rendering-my-solution/26824

	int test() {
#ifdef NDEBUG
		Uint32 flags = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL;	// | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#else
		Uint32 flags = 0;// SDL_WINDOW_OPENGL;	// | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#endif
		int frame = addWindow(1280, 960, 75, flags);
		if (frame < 0) {
			std::cout << "addWindow failure" << std::endl;
			return -1;
		}

		bool running = true;

		while (running) {
			clear(frame);
			int count = (frameCount++) % 100;
			int g = count * 2;
			flip(frame);
			SDL_Event event;
			if (SDL_WaitEventTimeout(&event, 5)) {
				switch (event.type) {
				case SDL_EVENT_KEY_DOWN:
					SDL_Keysym key = event.key.keysym;
					if (key.scancode == SDL_SCANCODE_ESCAPE) {
						running = false;
					}
					break;
				case SDL_EVENT_QUIT:
					running = false;
					break;
				}
			}
			SDL_Delay(5);
//			std::cout << "." << std::endl;
		}
		closeWindow(frame);
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

int testDriver(Driver* driver) {
	return driver->test();
}

int sdlMain() {
	std::cout << "plainview " << plainviewVersion << std::endl;
	Driver *sdlDriver = sdlOpen();
	dumpModes();
	testDriver(sdlDriver);
	sdlDriver->quit();
	return 0;
}

int main() {
	return sdlMain();
}
