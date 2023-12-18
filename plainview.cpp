/*

plainview.cpp

A plain view of the modern monitor video display landscape

Copyright © 2023 Simon Armstrong

All Rights Reserved

*/

const char *plainviewVersion = "0.3";

static bool terminateApp = false;

#include <iostream>
#include <map>
#include <vector>
#include <set>

#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"

#include <SDL3/SDL.h>
//#include <SDL3/SDL_opengl.h>
#define _USE_MATH_DEFINES
#include <math.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <profileapi.h>
#endif
#ifdef __APPLE__
#include <sys/time.h>
#endif

uint64_t cpuTime() {
	uint64_t micros;
#ifdef WIN32
	LARGE_INTEGER frequency;
	LARGE_INTEGER count;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&count);
	micros = (count.QuadPart * (uint64_t)1e6) / frequency.QuadPart;
#endif
#ifdef __APPLE__
	timeval tv;
	gettimeofday( &tv, nullptr );
	micros=tv.tv_sec*1000000ULL +tv.tv_usec;
#endif
	return micros;
}

struct Rect32 {
	int x,y,w,h;
	Rect32() {
		x = y = w = h = 0;
	}
	Rect32(int left,int top,int width,int height) {
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
typedef Rect32 R;

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

struct QuadDriver : Driver {
	virtual void drawQuad(int frame, R dest, RGBA c) {

	}
};

#include <fstream>
#include <sstream>

typedef std::string S;

S loadString(S path) {
	std::ifstream ifs(path);
	if (!ifs.is_open()) {
		return "";
	}
	std::stringstream buffer;
	buffer << ifs.rdbuf();
	return buffer.str();
}


typedef GLint i32;
typedef GLuint u32;
typedef GLchar c8;
typedef GLenum E;



struct GLProgram {

	i32 program1 = 0;

	int check() {
		E error = glGetError();
		if (error) {
			std::cout << "GL Error " << error << std::endl;
			abort();
		}
		return error;
	}

	i32 attribute( const char *name) {
		i32 a = glGetAttribLocation(program1, name);
		check();
		return a;
	}

	i32 uniform(const char* name) {
		i32 a = glGetUniformLocation(program1, name);
		check();
		return a;
	}


	int build() {
		program1 = loadProgram();
		i32 a0 = attribute("xyzc");
		i32 u0 = uniform("handles");
		i32 u1 = uniform("view");
		i32 u2 = uniform("palette");
		return 0;
	}

	i32 loadProgram() {
		std::string vertexGles = loadString("../../shaders/rayVertex.glsl");
		i32 shader1 = loadShader(GL_VERTEX_SHADER, vertexGles.data(), vertexGles.length());
		check();
		std::string fragmentGles = loadString("../../shaders/rayFragment.glsl");
		i32 shader2 = loadShader(GL_FRAGMENT_SHADER, fragmentGles.data(), fragmentGles.length());
		check();

		i32 program1 = glCreateProgram();
		glAttachShader(program1, shader1);
		glAttachShader(program1, shader2);
		check();

		glLinkProgram(program1);
		check();

		int status[4];
		glGetProgramiv(program1, GL_LINK_STATUS, status);

		if (status[0] == 0) {
			int status2[4];
			glGetProgramiv(program1, GL_INFO_LOG_LENGTH, status2);
			if (status2[0]) {
				i32 size = 0;
				c8 log[4096];
				glGetProgramInfoLog(program1, status2[0], &size, log);
				if (size) {
					std::cout << "program info log : " << std::string(log, size) << std::endl;
				}
			}
			glDeleteProgram(program1);
			return 0;
		}

		return program1;
	}

	i32 loadShader(GLenum shaderType,char *src, int bytes) {
		u32 shader = glCreateShader(shaderType);
		const c8* sources[] = { src,0 };
		i32 lengths[] = {bytes,0};
		glShaderSource(shader, 1, sources, lengths);
		glCompileShader(shader);
		check();
		int status[4];
		glGetShaderiv(shader, GL_COMPILE_STATUS, status);
		if (status[0] == 0) {
			int status2[4];
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, status2);
			if (status2[0]) {
				i32 size = 0;
				c8 log[4096];
				glGetShaderInfoLog(shader, status2[0], &size, log);
				if (size) {
					std::cout << "shader info log : " << std::string(log, size) << std::endl;
				}
			}
			glDeleteShader(shader);
			return 0;
		}
		check();
		return shader;
	}

};

struct GLEngine {

	int test() {

        const char* version = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
        std::cout << "GL_SHADING_LANGUAGE_VERSION : " << version << std::endl;

		GLProgram p;
		p.build();
		return 0;
	}
};

/*
constructor(gl, source, shaderType, uri) {
			const shader = gl.createShader(shaderType);
			gl.shaderSource(shader, source);
			gl.compileShader(shader);
			const success = gl.getShaderParameter(shader, gl.COMPILE_STATUS);
			if (success) {
				this.shader = shader;
			}
			else {
				const info = gl.getShaderInfoLog(shader);
				console.error("Could not compile WebGL shader", uri);
				console.error(info);
				const split = info.split(":");
				if (split[0] == "ERROR") {
					this.error = split;
				}
				gl.deleteShader(shader);
			}
		}
*/

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
        
        
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
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

	// todo: assert ( flags & SDL_WINDOW_OPENGL )
	
	int addWindow(int w, int h, int freq, int window_flags) {
		SDL_Window* window = NULL;
		window = SDL_CreateWindow("plainview", w, h, window_flags);
		if (window == NULL) {
			const char* error = SDL_GetError();
			std::cout << "could not create window " << error << std::endl;
			return -1;
		}
		
		SDL_GLContext glContext = SDL_GL_CreateContext(window);
		int version = gladLoadGL((GLADloadfunc) SDL_GL_GetProcAddress);
		SDL_GL_SetSwapInterval(1);
		int frameIndex = (int)frames.size();
		frames.push_back({ window,NULL,NULL,NULL});
		return frameIndex;
	}
	
	void clear(int frame) {
	}

	void drawQuad(int frame, R dest, RGBA c) {
		SDLFrame& sdlFrame = frames[frame];
	}

	static const int MAX_RECT = 8192;

	void drawQuads(int frame, R *dests, RGBA c,const int count) {
		SDLFrame& sdlFrame = frames[frame];
	}

	void drawPlatter(int frame, int x, int y, int c, int radius, int denominator, int depth, float prot)
	{
		double rr = 2 * M_PI / denominator;
		for (int i = 0; i < denominator; i++) {
			for (int d = 0; d < depth; d++) {
				int r = radius + d * 6;
				int tx = x + r * sin(rr * i - prot);
				int ty = y + r * cos(rr * i - prot);
				R dest = { tx,ty,4,2 };
				drawQuad(frame, dest, c);
			}
		}
	}

	void flip(int frame){
		SDLFrame& sdlFrame = frames[frame];
		SDL_Window* window = sdlFrame.window;
		int w, h;
		SDL_GetWindowSizeInPixels(window, &w, &h);
		int period = 23;
		int x = 10 + frameCount % period;
		glClearColor(0.1f, 0.f, 0.4f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_SCISSOR_TEST);
		while (x < w) {
			glScissor(x, 0, 2, h);
			glClearColor(0.8f, 0.8f, 1.f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT);
			x += period;
		}
		glDisable(GL_SCISSOR_TEST);
		SDL_GL_SwapWindow(window);
	}

	void fullscreen(int frame) {
		SDLFrame& sdlFrame = frames[frame];
		SDL_Window* window = sdlFrame.window;
		SDL_SetWindowFullscreen(window, true);
	}
	
	// https://discourse.libsdl.org/t/vsync-while-software-rendering-my-solution/26824

	int fps;
	int fpsFrames=0;
	uint64_t frameTime=cpuTime();
		
	void updateFPS(){
		fpsFrames++;
		if(fpsFrames>=40){
			uint64_t t=cpuTime();
			double elapsed = (t-frameTime)/1e6;
			frameTime=t;
			fps= (0.5 + (fpsFrames / elapsed));
			fpsFrames=0;
			std::cout << "fps:" << fps << std::endl;
		}
	}

	int test() {
		Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;    // | SDL_WINDOW_HIGH_PIXEL_DENSITY;
/*
#ifdef NDEBUG
		Uint32 flags = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL;	// | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#else
		Uint32 flags = SDL_WINDOW_OPENGL;	// | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#endif
*/
//		int frame = addWindow(1280, 960, 75, flags);
		int frame = addWindow(1280, 800, 75, flags);
//		int frame = addWindow(3024, 1964, 120, flags);
		if (frame < 0) {
			std::cout << "addWindow failure" << std::endl;
			return -1;
		}


		GLEngine e;
		e.test();

		bool running = true;

		int surface = frame;

		while (running) {
//			clear(surface);
			int count = (frameCount++) % 100;
			int g = count * 2;
			flip(surface);
			
			updateFPS();

			SDL_Event event;
//            if (SDL_WaitEventTimeout(&event, 5)) {
			if (SDL_PollEvent(&event)) {
				switch (event.type) {
				case SDL_EVENT_WINDOW_MAXIMIZED: {
					fullscreen(0);
				}break;
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
