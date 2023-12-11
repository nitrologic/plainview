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
#define _USE_MATH_DEFINES
#include <math.h>

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
	VideoMode(const VideoMode& other) {
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
		SDL_Texture* texture;
	};

	std::vector<SDLFrame> frames;

	void closeWindow(int surface) {
		SDLFrame& sdlFrame = frames[surface];
		SDL_Window* window = sdlFrame.window;
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	int addWindow2(int w, int h, int freq, int window_flags) {
		SDL_Window* window = NULL;
		window = SDL_CreateWindow("plainview", w, h, window_flags);
		if (window == NULL) {
			const char* error = SDL_GetError();
			std::cout << "could not create window " << error << std::endl;
			return -1;
		}
		const char* name = "nitrologic";

		SDL_Renderer* renderer = SDL_CreateRenderer(window, name, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
		SDL_Surface * surface = SDL_GetWindowSurface(window);

		//window->format->format

		Uint32 format = 0;
		SDL_Texture * texture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_TARGET, w, h);

//		int vsync = SDL_SetWindowTextureVSync(window, 1);

		int vsync = SDL_SetRenderVSync(renderer, 1);

		int frameIndex = (int)frames.size();
		frames.push_back({ window,surface,renderer,texture });
		return frameIndex;
	}

	int addWindow(int w, int h, int freq, int window_flags) {
		SDL_Window* window = NULL;
		window = SDL_CreateWindow("plainview", w, h, window_flags);
		if (window == NULL) {
			const char* error = SDL_GetError();
			std::cout << "could not create window " << error << std::endl;
			return -1;
		}
		SDL_Surface* surface = SDL_GetWindowSurface(window);
		int frameIndex = (int)frames.size();
		frames.push_back({ window,surface,NULL,NULL });
		return frameIndex;
	}

	void clear(int frame) {
		SDLFrame& sdlFrame = frames[frame];
		SDL_Surface* surface = sdlFrame.surface;
		SDL_Renderer* renderer = sdlFrame.renderer;
		if (renderer) {
			SDL_RenderClear(renderer);
			return;
		}

		RGBA bg = 0x00000000;
		if (surface) {
			SDL_FillSurfaceRect(surface, NULL, bg);
		}
	}

	void drawQuad(int frame, R dest, RGBA c) {
		SDLFrame& sdlFrame = frames[frame];
		SDL_Surface* surface = sdlFrame.surface;

		SDL_Renderer* renderer = sdlFrame.renderer;
		if (renderer) {
			SDL_FRect frect = { (float)dest.x,(float)dest.y,(float)dest.w,(float)dest.h };
			SDL_SetRenderDrawColor(renderer, 250, 14, 23, 255);
			SDL_RenderFillRect(renderer, &frect);
		}

		SDL_Rect rect = { dest.x,dest.y,dest.w,dest.h };
		SDL_FillSurfaceRect(surface, &rect, c);
	}

	static const int MAX_RECT = 8192;

	void drawQuads(int frame, R *dests, RGBA c,const int count) {
		SDLFrame& sdlFrame = frames[frame];
		SDL_Surface* surface = sdlFrame.surface;

		SDL_Renderer* renderer = sdlFrame.renderer;
		if (renderer) {
			SDL_FRect frect[MAX_RECT];
			for (int i = 0; i < count; i++) {
				frect[i].x = dests->x;
				frect[i].y = dests->y;
				frect[i].w = dests->w;
				frect[i].h = dests->h;
				dests++;
			}
			SDL_SetRenderDrawColor(renderer, 250, 14, 23, 255);
			SDL_RenderFillRects(renderer, frect, count);
		} 
		else 
		{
			SDL_Rect rect[MAX_RECT];
			for (int i = 0; i < count; i++) {
				rect[i].x = dests->x;
				rect[i].y = dests->y;
				rect[i].w = dests->w;
				rect[i].h = dests->h;
				dests++;
			}
			SDL_FillSurfaceRects(surface, rect, count, c);
		}
	}


	void drawPlatter(int frame, int x, int y, int c, int radius, int denominator, int depth, float prot)
	{
		double rr = 2 * M_PI / denominator;
		for (int i = 0; i < denominator; i++) {
			for (int d = 0; d < depth; d++) {
				int r = radius + d * 6;
				int tx = x + r * sin(rr * i + prot);
				int ty = y + r * cos(rr * i + prot);
				R dest = { tx,ty,4,2 };
				drawQuad(frame, dest, c);
			}
		}
	}

	void flip2(int frame) {
		SDLFrame& sdlFrame = frames[frame];
		SDL_Window* window = sdlFrame.window;
		SDL_Renderer* renderer = sdlFrame.renderer;
		SDL_Texture* texture = sdlFrame.texture;


		int x = 10 + frameCount % 40;
//		drawQuad(frame, { x * 10,10,4,4 }, 0xff44cc44);
//		SDL_Texture* texture = sdlFrame.texture;
//		int w = frameWidth(frame);
//		int h = frameHeight(frame);
//		SDL_RenderTexture(renderer, texture, NULL, NULL);
//		SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
		SDL_RenderClear(renderer);
		SDL_SetRenderDrawColor(renderer, 250, 14, 23, 255);
//		const SDL_FPoint points[] = { 0,0,-10,10,10,0,0,-10 };
//		int draw = SDL_RenderLines(renderer, points, 4);
		SDL_FRect rect = {20.0f+x, 20.0f+x, 100.0f, 100.0f};
		SDL_RenderFillRect(renderer, &rect);

		SDL_RenderTexture(renderer, texture, NULL, NULL);

		SDL_RenderPresent(renderer);
	}

	void flip(int frame) {
		SDLFrame& sdlFrame = frames[frame];
		SDL_Window* window = sdlFrame.window;
		SDL_Renderer* renderer = sdlFrame.renderer;

		int w, h;
		SDL_GetWindowSize(window, &w, &h);
		float prot = M_PI_2 * (frameCount & 127) / 256.0;

		drawPlatter(frame, 300, 400, -1, 240, 1024 / 8, 8, prot);
		drawPlatter(frame, w - 300, 400, -1, 240, 1024 / 8, 8, prot);
		drawPlatter(frame, 300, 400, -1, 140, 256 / 32, 2, prot);
		drawPlatter(frame, w - 300, 400, -1, 140, 256 / 32, 2, prot);

		if (renderer)
		{
			SDL_Texture* texture = sdlFrame.texture;
			SDL_RenderTexture(renderer, texture, NULL, NULL);
			SDL_RenderPresent(renderer);
			return;
		}
		SDL_UpdateWindowSurface(window);
	}

	// https://discourse.libsdl.org/t/vsync-while-software-rendering-my-solution/26824

	int test() {
#ifdef NDEBUG
		Uint32 flags = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL;	// | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#else
		Uint32 flags = 0;// SDL_WINDOW_OPENGL;	// | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#endif
//		int frame = addWindow(1280, 960, 75, flags);
		int frame = addWindow2(1280, 960, 75, flags);
		if (frame < 0) {
			std::cout << "addWindow failure" << std::endl;
			return -1;
		}

		bool running = true;

		int surface = frame;

		while (running) {
			clear(surface);
			int count = (frameCount++) % 100;
			int g = count * 2;
			flip(surface);

			SDL_Event event;
			if (SDL_WaitEventTimeout(&event, 5)) {
				switch (event.type) {
				case SDL_EVENT_KEY_DOWN:{
					SDL_Keysym key = event.key.keysym;
					if (key.scancode == SDL_SCANCODE_ESCAPE) {
						running = false;
					}
					}break;
				case SDL_EVENT_QUIT:
					running = false;
					break;
				}
			}
			SDL_Delay(2);
//			std::cout << "." << std::endl;
		}

		closeWindow(frame);
	}
};



struct SDLDriver2 : Driver {

	Uint32 sdlFlags;

	SDLDriver2() {
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
		Uint32 flags = SDL_WINDOW_OPENGL;	// | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#endif
//		int frame = addWindow(1280, 960, 75, flags);
		int frame = addWindow(1440, 900, 0, flags);
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
				case SDL_EVENT_KEY_DOWN:{
					SDL_Keysym key = event.key.keysym;
					if (key.scancode == SDL_SCANCODE_ESCAPE) {
						running = false;
					}
					}break;
				case SDL_EVENT_QUIT:
					running = false;
					break;
				}
			}
			SDL_Delay(5);
			std::cout << "." << std::endl;
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
